import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
import {createHash, webcrypto} from 'node:crypto';
import {setTimeout as delay} from 'node:timers/promises';

const source=fs.readFileSync(new URL('../web/asset-loader.js',import.meta.url),'utf8');
const hash=data=>createHash('sha256').update(data).digest('hex');
function memoryDatabase() {
  const stores=new Map();
  return {open() {
    const opening={};
    setTimeout(()=>{
      opening.result={close(){},createObjectStore(name){stores.set(name,new Map());},transaction(names){
        const tx={}; let timer;
        const schedule=()=>{clearTimeout(timer);timer=setTimeout(()=>tx.oncomplete?.(),0);};
        tx.objectStore=name=>({
          get(key){const r={};setTimeout(()=>{r.result=stores.get(name).get(key);r.onsuccess?.();schedule();},0);return r;},
          getAll(){const r={};setTimeout(()=>{r.result=[...stores.get(name).values()];r.onsuccess?.();schedule();},0);return r;},
          put(value,key){stores.get(name).set(key,value);schedule();},
          delete(key){stores.get(name).delete(key);schedule();},
        }); return tx;
      }};
      if (!stores.size) opening.onupgradeneeded?.();
      opening.onsuccess?.();
    },0); return opening;
  }};
}
function harness({data=Buffer.from('level pixels'),storage,crypto}={}) {
  const digest=hash(data), path='resources/environment/level.png', url=`assets/${digest}.png`;
  const calls=[], written=new Map(); let response=data, fail=false;
  const context={console:{warn(){}},setTimeout,clearTimeout,URL,AbortController,Uint8Array,Uint32Array,DataView,Map,
    document:{baseURI:'http://192.168.4.35/games/floppy-turd/releases/test/index.html'},
    location:{href:'http://192.168.4.35/games/floppy-turd/releases/test/index.html'},
    indexedDB:storage,crypto,
    floppyAssetCatalog:{schemaVersion:1,files:{[path]:{url,bytes:data.byteLength,sha256:digest}},groups:{'level-0':[path]}},
    Module:{FS:{mkdirTree(){},writeFile(path,bytes){written.set(path,Buffer.from(bytes));},unlink(path){written.delete(path);}}},
    fetch:async url=>{calls.push(String(url));if(fail)throw Error('offline');return {ok:true,arrayBuffer:async()=>Uint8Array.from(response).buffer};},
  };
  context.window=context;
  vm.runInNewContext(source,context);
  return {api:context.floppyAssets,calls,written,path,setResponse(value){response=value;},setFail(value){fail=value;}};
}
async function settled(h) {
  for(let i=0;i<100 && h.api.summary().pending;i++) await delay(5);
  assert.equal(h.api.summary().pending,0,'load did not settle');
}
test('no eager fetch; level assets use immutable release-relative URLs; repeated requests deduplicate',async()=>{
  const h=harness();assert.equal(h.calls.length,0);
  assert.equal(h.api.group('level-0'),0);assert.equal(h.api.file(h.path),0);
  await settled(h);
  assert.equal(h.api.group('level-0'),1);assert.equal(h.calls.length,1);
  assert.match(h.calls[0],/\/releases\/test\/assets\/[a-f0-9]{64}\.png$/);
  assert.equal(h.written.get('/'+h.path).toString(),'level pixels');
});
test('HTTP LAN SHA256 fallback rejects corrupted bytes, then explicit retry succeeds',async()=>{
  const h=harness();h.setResponse(Buffer.from('wrong pixels'));
  h.api.group('level-0');await settled(h);
  assert.equal(h.api.group('level-0'),-1);assert.equal(h.written.size,0);
  h.setResponse(Buffer.from('level pixels'));h.api.retryGroup('level-0');await settled(h);
  assert.equal(h.api.group('level-0'),1);
});
test('SHA256 fallback handles padding boundaries and large music; HTTPS digest also works',async()=>{
  for(const length of [0,1,55,56,63,64,65,1000000]) {
    const h=harness({data:Buffer.alloc(length,0xab)});h.api.file(h.path);await settled(h);
    assert.equal(h.api.file(h.path),1,`SHA256 length${length}`);
  }
  const secure=harness({crypto:webcrypto});secure.api.file(secure.path);await settled(secure);
  assert.equal(secure.api.file(secure.path),1);
});
test('cached files survive a new page instance and work offline; release frees MEMFS',async()=>{
  const storage=memoryDatabase();const first=harness({storage});
  first.api.file(first.path);await settled(first);await delay(15);
  first.api.release(first.path);assert.equal(first.written.size,0);
  const second=harness({storage});second.setFail(true);second.api.file(second.path);await settled(second);
  assert.equal(second.api.file(second.path),1);assert.equal(second.calls.length,0);
});
test('cancelled downloads cannot install a file into a later scene',async()=>{
  const h=harness();h.api.file(h.path);h.api.release(h.path);await delay(10);
  assert.equal(h.written.size,0);assert.equal(h.api.summary().ready,0);
});
