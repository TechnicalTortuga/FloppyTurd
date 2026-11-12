(() => {
  'use strict';
  // IndexedDB works on LAN HTTP too; CacheStorage/SubtleCrypto require HTTPS.
  // Failure/eviction/private browsing falls back to ordinary HTTP caching.
  const limit = 64 * 1024 * 1024;
  const base = document.baseURI || location.href;
  const states = new Map();
  let database;
  function db() {
    if (!database) database = new Promise(resolve => {
      if (!window.indexedDB) return resolve(null);
      let settled = false;
      const finish = value => { if (!settled) { settled=true; clearTimeout(timer); resolve(value); } else value?.close(); };
      const timer = setTimeout(() => finish(null), 1500);
      try {
        const opening = indexedDB.open('floppy-turd-assets-v1', 1);
        opening.onupgradeneeded = () => { opening.result.createObjectStore('files'); opening.result.createObjectStore('meta'); };
        opening.onsuccess = () => finish(opening.result);
        opening.onerror = opening.onblocked = () => finish(null);
      } catch { finish(null); }
    });
    return database;
  }
  async function cached(key) {
    try {
      const store = await db(); if (!store) return null;
      const tx = store.transaction(['files','meta'], 'readwrite');
      const read = tx.objectStore('files').get(key);
      read.onsuccess = () => { if (read.result) tx.objectStore('meta').put({key,bytes:read.result.byteLength,touched:Date.now()}, key); };
      return await new Promise((resolve,reject) => { tx.oncomplete=()=>resolve(read.result || null); tx.onerror=tx.onabort=()=>reject(tx.error); });
    } catch { return null; }
  }
  async function remember(key, data) {
    try {
      const store = await db(); if (!store || data.byteLength > limit) return;
      const tx = store.transaction(['files','meta'], 'readwrite');
      tx.objectStore('files').put(data, key);
      tx.objectStore('meta').put({key,bytes:data.byteLength,touched:Date.now()}, key);
      const all = tx.objectStore('meta').getAll();
      all.onsuccess = () => {
        const records=all.result.sort((a,b)=>a.touched-b.touched);
        let total=records.reduce((sum,item)=>sum+item.bytes,0);
        for (const item of records) {
          if (total <= limit) break;
          tx.objectStore('files').delete(item.key); tx.objectStore('meta').delete(item.key); total-=item.bytes;
        }
      };
      await new Promise(resolve => { tx.oncomplete=tx.onabort=tx.onerror=resolve; });
    } catch { /* Storage is an optimization, never a prerequisite for playing. */ }
  }
  const constants = [0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2];
  const rotate = (n,bits) => n>>>bits | n<<(32-bits);
  function fallbackHash(data) {
    const input=new Uint8Array(data), length=Math.ceil((input.length+9)/64)*64;
    const padded=new Uint8Array(length); padded.set(input); padded[input.length]=0x80;
    const view=new DataView(padded.buffer);
    view.setUint32(length-8,Math.floor(input.length/0x20000000)); view.setUint32(length-4,input.length*8);
    const hash=[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19], words=new Uint32Array(64);
    for (let offset=0;offset<length;offset+=64) {
      for (let i=0;i<16;i++) words[i]=view.getUint32(offset+i*4);
      for (let i=16;i<64;i++) {
        const x=words[i-15],y=words[i-2];
        words[i]=words[i-16]+(rotate(x,7)^rotate(x,18)^x>>>3)+words[i-7]+(rotate(y,17)^rotate(y,19)^y>>>10);
      }
      let [a,b,c,d,e,f,g,h]=hash;
      for (let i=0;i<64;i++) {
        const t1=(h+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+constants[i]+words[i])|0;
        const t2=((rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c)))|0;
        h=g;g=f;f=e;e=(d+t1)|0;d=c;c=b;b=a;a=(t1+t2)|0;
      }
      [a,b,c,d,e,f,g,h].forEach((value,i)=>hash[i]=(hash[i]+value)>>>0);
    }
    return hash.map(value=>value.toString(16).padStart(8,'0')).join('');
  }
  async function digest(data) {
    if (window.crypto?.subtle) return [...new Uint8Array(await crypto.subtle.digest('SHA-256',data))].map(x=>x.toString(16).padStart(2,'0')).join('');
    return fallbackHash(data);
  }
  function notify() { window.floppyAssetStateChanged?.(); }
  function file(path) {
    const record=window.floppyAssetCatalog?.files[path];
    if (!record) return -1;
    const previous=states.get(path); if (previous) return previous.status;
    const state={status:0,controller:new AbortController()}; states.set(path,state);
    const key=record.sha256;
    state.promise=(async () => {
      let data=await cached(key);
      if (data && (data.byteLength !== record.bytes || await digest(data) !== record.sha256)) data=null;
      if (!data) {
        for (const cache of ['force-cache','reload']) {
          const response=await fetch(new URL(record.url,base),{signal:state.controller.signal,cache});
          if (!response.ok) throw new Error(`HTTP ${response.status}`);
          data=await response.arrayBuffer();
          if (data.byteLength===record.bytes && await digest(data)===record.sha256) break;
          data=null; // A corrupt HTTP cache gets one network-only recovery attempt.
        }
        if (!data) throw new Error('Asset integrity check failed');
        void remember(key,data);
      }
      if (states.get(path)!==state) return;
      const virtual='/'+path;
      Module.FS.mkdirTree(virtual.slice(0,virtual.lastIndexOf('/')));
      Module.FS.writeFile(virtual,new Uint8Array(data),{canOwn:true});
      state.status=1; notify();
    })().catch(error => {
      if (states.get(path)!==state) return;
      state.status=-1; state.error=String(error); notify();
      console.warn('Asset unavailable:',path,error);
    });
    return 0;
  }
  function release(path) {
    const state=states.get(path); if (!state) return;
    states.delete(path); state.controller.abort();
    if (state.status===1) { try { Module.FS.unlink('/'+path); } catch {} }
    notify();
  }
  function group(name) {
    const paths=window.floppyAssetCatalog?.groups[name]; if (!paths) return -1;
    const results=paths.map(file);
    return results.includes(-1) ? -1 : results.every(status=>status===1) ? 1 : 0;
  }
  function retryGroup(name) {
    for (const path of window.floppyAssetCatalog?.groups[name] || []) if (states.get(path)?.status===-1) states.delete(path);
    return group(name);
  }
  window.floppyAssets=Object.freeze({file,release,group,retryGroup,
    retryFailed() { for (const [path,state] of states) if (state.status===-1) { states.delete(path); file(path); } },
    summary() { return {pending:[...states.values()].filter(s=>s.status===0).length,failed:[...states.values()].filter(s=>s.status===-1).length,ready:[...states.values()].filter(s=>s.status===1).length,cacheLimitBytes:limit}; },
  });
})();
