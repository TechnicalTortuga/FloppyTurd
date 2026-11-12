import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const source = fs.readFileSync(new URL('../web/bridge.js', import.meta.url), 'utf8');
function harness() {
  const listeners = {}, sent = [], calls = [], elements = {};
  for (const name of ['canvas','gate','play','status','hint','fullscreen','mobile-top','mobile-bottom','touch-back','touch-pause','touch-jump','touch-shoot']) elements[name] = { hidden:false, textContent:'', clientWidth:960, clientHeight:540, addEventListener(name, fn) { this[name] = fn; }, focus() {} };
  const parent = { postMessage: (message, origin) => sent.push({message, origin}) };
  const context = { console, URL, URLSearchParams, Uint8Array,
    location: { search:'?embed=1&parentOrigin=https%3A%2F%2Fportfolio.example&session=abc' }, parent,
    document: {hidden:false, getElementById: id => elements[id], addEventListener(name, fn) {listeners[name] = fn;} },
    ResizeObserver: class { observe() {} },
    addEventListener(name, fn) {listeners[name] = fn;},
  };
  context.window = context;
  vm.runInNewContext(source, context);
  context.Module.ccall = (...args) => calls.push(args);
  context.Module.callMain = () => {};
  const envelope = (type, payload) => ({source:parent, origin:'https://portfolio.example', data:{channel:'floppy-turd',version:1,session:'abc',type,payload}});
  return { context, listeners, sent, calls, elements, envelope };
}
test('embedded ready/hidden/resume/dispose flow reaches the game', () => {
  const h = harness();
  h.listeners.message(h.envelope('host:init', {visible:true}));
  h.context.Module.onRuntimeInitialized();
  assert.equal(h.sent[0].message.type, 'game:ready');
  assert.equal(h.sent[0].origin, 'https://portfolio.example');
  h.elements.play.click();
  assert.equal(h.calls.at(-2)[0], 'floppy_set_paused');
  h.listeners.message(h.envelope('host:state', {visible:false}));
  assert.equal(h.calls.at(-2)[3][0], 1);
  h.listeners.message(h.envelope('host:state', {visible:true}));
  assert.equal(h.calls.at(-2)[3][0], 0);
  h.listeners.message(h.envelope('host:dispose', {reason:'exit'}));
  assert.equal(h.calls.at(-2)[3][0], 1);
});
test('wrong origin, source, session and protocol cannot change visibility', () => {
  const h = harness(); h.context.Module.onRuntimeInitialized(); h.elements.play.click();
  const count = h.calls.length;
  for (const mutate of [e=>e.origin='https://evil.example', e=>e.source={}, e=>e.data.session='stale', e=>e.data.version=2, e=>e.data.channel='other']) {
    const e=h.envelope('host:state',{visible:true}); mutate(e); h.listeners.message(e);
  }
  assert.equal(h.calls.length,count);
});
test('embedded F/Escape route to host while modified and editable keys pass through', () => {
  const h = harness();
  function key(code, extra={}) { h.listeners.keydown({code,target:{},preventDefault(){},stopImmediatePropagation(){},...extra}); }
  key('KeyF'); key('Escape');
  assert.deepEqual(h.sent.map(e=>e.message.type), ['game:request-fullscreen','game:request-close']);
  key('KeyF',{ctrlKey:true}); key('KeyF',{metaKey:true}); key('Escape',{target:{isContentEditable:true}});
  assert.equal(h.sent.length,2);
  assert.match(h.elements.hint.textContent,/return to website/);
});

test('touch controls support tap jump, held shooting, pause, and cancellation', () => {
  const h=harness(); h.listeners.message(h.envelope('host:init',{visible:true})); h.context.Module.onRuntimeInitialized(); h.elements.play.click();
  h.listeners.pointerdown({pointerType:'touch'});
  h.context.floppySetUiState(3);
  assert.equal(h.elements['mobile-bottom'].hidden,false);
  assert.equal(h.elements['touch-shoot'].hidden,false);
  const pointer=(id,type='touch')=>({pointerId:id,pointerType:type,preventDefault(){},stopImmediatePropagation(){}});
  h.elements.canvas.pointerdown(pointer(1));
  assert.equal(h.calls.at(-1)[0],'floppy_touch'); assert.deepEqual(Array.from(h.calls.at(-1)[3]),[0,1]);
  h.elements['touch-shoot'].pointerdown(pointer(2));
  assert.deepEqual(Array.from(h.calls.at(-1)[3]),[1,1]);
  h.elements.canvas.pointerup(pointer(1));
  h.elements['touch-shoot'].pointercancel(pointer(2));
  assert.deepEqual(Array.from(h.calls.at(-1)[3]),[1,0]);
  h.elements['touch-pause'].pointerdown(pointer(3));
  assert.deepEqual(Array.from(h.calls.at(-1)[3]),[2,1]);
  h.context.floppySetUiState(5);
  assert.equal(h.elements['touch-pause'].textContent,'Resume');
  assert.equal(h.elements['mobile-bottom'].hidden,true);
});
test('touch controls stay hidden in menus and locked shooting is not offered', () => {
  const h=harness(); h.listeners.message(h.envelope('host:init',{visible:true})); h.context.Module.onRuntimeInitialized(); h.elements.play.click();
  h.listeners.pointerdown({pointerType:'pen'});
  h.context.floppySetUiState(0); assert.equal(h.elements['mobile-bottom'].hidden,true);
  h.context.floppySetUiState(1); assert.equal(h.elements['touch-shoot'].hidden,true);
  h.context.floppySetUiState(9); assert.equal(h.elements['touch-pause'].hidden,true);
});
