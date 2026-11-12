import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
const source = fs.readFileSync(new URL('../web/bridge.js', import.meta.url), 'utf8');
function harness(extra = "") {
  const listeners = {}, sent = [], calls = [], elements = {};
  for (const name of ['canvas','gate','play','status','hint','fullscreen','mobile-top','mobile-bottom','touch-back','touch-pause','touch-jump','touch-shoot']) elements[name] = { hidden:false, textContent:'', clientWidth:960, clientHeight:540, addEventListener(name, fn) { this[name] = fn; }, focus() {} };
  elements.canvas.getContext = function(type,attributes) { this.contextAttributes=attributes; return {}; };
  const parent = { postMessage: (message, origin) => sent.push({message, origin}) };
  const context = { console, URL, URLSearchParams, Uint8Array,
    location: { search:'?embed=1&parentOrigin=https%3A%2F%2Fportfolio.example&session=abc'+extra }, parent,
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
  assert.ok(h.calls.some(call=>call[0]==='floppy_set_paused'));
  h.listeners.message(h.envelope('host:state', {visible:false}));
  assert.equal(h.calls.filter(call=>call[0]==='floppy_set_paused').at(-1)[3][0], 1);
  h.listeners.message(h.envelope('host:state', {visible:true}));
  assert.equal(h.calls.filter(call=>call[0]==='floppy_set_paused').at(-1)[3][0], 0);
  h.listeners.message(h.envelope('host:dispose', {reason:'exit'}));
  assert.equal(h.calls.filter(call=>call[0]==='floppy_set_paused').at(-1)[3][0], 1);
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
  key('KeyF'); key('Escape',{shiftKey:true});
  assert.deepEqual(h.sent.map(e=>e.message.type), ['game:request-fullscreen','game:request-close']);
  key('KeyF',{ctrlKey:true}); key('KeyF',{metaKey:true}); key('Escape',{target:{isContentEditable:true}});
  assert.equal(h.sent.length,2);
  assert.match(h.elements.hint.textContent,/Shift\+Esc: return to website/);
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

const modifiers={shift:false,ctrl:false,alt:false,meta:false};
const portfolioKey=(code,phase='down',mods=modifiers)=>({schema:'portfolio-input',version:1,kind:'key',phase,code,repeat:false,modifiers:mods});
test('embedded autostart is gated only by runtime readiness and starts muted', () => {
  const h=harness('&autostart=1');
  h.listeners.message(h.envelope('host:init',{visible:true}));
  assert.equal(h.context.floppyStartMuted,true);
  h.context.Module.onRuntimeInitialized();
  assert.equal(h.elements.gate.hidden,true); assert.equal(h.elements.play.hidden,true);
  assert.ok(h.calls.some(c=>c[0]==='floppy_set_presentation' && c[3][0]===1));
  assert.ok(!h.calls.some(c=>c[0]==='floppy_activate_audio'));
  h.elements.canvas.getContext('webgl',{alpha:false});
  assert.equal(h.elements.canvas.contextAttributes.preserveDrawingBuffer,true);
});
test('portfolio input maps keys directly and plain Escape remains a game key', () => {
  const h=harness('&autostart=1'); h.context.Module.onRuntimeInitialized();
  assert.equal(h.context.floppyEmbed.input(portfolioKey('Escape')),true);
  assert.equal(h.calls.filter(c=>c[0]==='floppy_key').at(-1)[3][0],256);
  assert.ok(!h.sent.some(e=>e.message.type==='game:request-close'));
  h.context.floppyEmbed.input(portfolioKey('Escape','down',{...modifiers,shift:true}));
  assert.ok(h.sent.some(e=>e.message.type==='game:request-close'));
});
test('portfolio pointer validation and message delivery do not synthesize DOM events or unlock audio', () => {
  const h=harness('&autostart=1'); h.context.Module.onRuntimeInitialized();
  const pointer={schema:'portfolio-input',version:1,kind:'pointer',phase:'down',pointerId:1,pointerType:'mouse',x:0.5,y:0.5,button:0,buttons:1,pressure:0.5};
  assert.equal(h.context.floppyEmbed.input({...pointer,x:NaN}),false);
  assert.equal(h.context.floppyEmbed.input({...pointer,x:1.1}),false);
  h.listeners.message(h.envelope('host:input',pointer));
  assert.ok(h.calls.some(c=>c[0]==='floppy_pointer' && c[3][0]===1 && c[3][1]===0.5));
  assert.ok(!h.calls.some(c=>c[0]==='floppy_activate_audio'));
  h.context.floppyEmbed.input(pointer);
  assert.ok(h.calls.some(c=>c[0]==='floppy_activate_audio'));
  assert.equal(h.context.floppyEmbed.input({schema:'portfolio-input',version:1,kind:'reset',reason:'blur'}),true);
  assert.ok(h.calls.some(c=>c[0]==='floppy_reset_input'));
});
