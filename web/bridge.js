(() => {
  'use strict';
  const query = new URLSearchParams(location.search);
  const embedded = query.get('embed') === '1' && window.parent !== window;
  const autostart = embedded && query.get('autostart') === '1';
  window.floppyStartMuted = autostart;
  window.floppyIsEmbedded = embedded;
  const audioContexts = new Set();
  for (const name of ['AudioContext','webkitAudioContext']) {
    const Original = window[name];
    if (Original) window[name] = new Proxy(Original, {construct(target,args,newTarget) {
      const context = Reflect.construct(target,args,newTarget);
      audioContexts.add(context);
      context.addEventListener?.('statechange', updateAudioControl);
      return context;
    }});
  }
  const session = query.get('session') || '';
  const parentOrigin = query.get('parentOrigin') || '';
  const validParent = (() => { try { return new URL(parentOrigin).origin === parentOrigin; } catch { return false; } })();
  const canvas = document.getElementById('canvas');
  if (embedded && canvas.getContext) {
    const original = canvas.getContext.bind(canvas);
    canvas.getContext = (type, attributes) => original(type, /^(webgl2?|experimental-webgl)$/.test(type) ? {...attributes,preserveDrawingBuffer:true} : attributes);
  }
  const gate = document.getElementById('gate');
  const play = document.getElementById('play');
  const fullscreen = document.getElementById('fullscreen');
  function requestFullscreen() {
    if (document.fullscreenElement) document.exitFullscreen?.().catch(console.warn);
    else document.documentElement?.requestFullscreen?.().catch(() => {});
  }
  fullscreen.hidden = embedded;
  fullscreen.addEventListener('click', requestFullscreen);
  const status = document.getElementById('status');
  document.getElementById('hint').textContent = embedded ? 'Space: jump · S: shoot · Tab / Esc: pause · Shift+Esc: return to website' : 'Space: jump · S: shoot · Tab / Esc: pause';
  let initialized = false;
  let started = false;
  let disposed = false;
  let inlinePresentation = autostart;
  let state = { visible: !embedded };
  let touchMode = query.get('touch') === '1' || (window.navigator?.maxTouchPoints || 0) > 0 || !!window.matchMedia?.('(any-pointer: coarse)').matches;
  let gameUiState = 0;
  let appliedTouchMode = null;
  const mobileTop = document.getElementById('mobile-top');
  const mobileBottom = document.getElementById('mobile-bottom');
  const backButton = document.getElementById('touch-back');
  const pauseButton = document.getElementById('touch-pause');
  const jumpButton = document.getElementById('touch-jump');
  const shootButton = document.getElementById('touch-shoot');
  const audioButton = document.getElementById('enable-audio');
  const chrome = document.getElementById('browser-chrome');
  const safeProbe = document.getElementById('safe-area');
  const rotationGate = document.getElementById('rotation-gate');
  const rotationTitle = document.getElementById('rotation-title');
  const rotationDescription = document.getElementById('rotation-description');
  let orientationWaiting = false;
  let lastLayout = '';
  const assetError = document.getElementById('asset-error');
  function updateAudioControl() {
    if (!audioButton) return;
    const live=[...audioContexts].filter(context=>context.state!=='closed');
    audioButton.hidden = !started || disposed || inlinePresentation || !state.visible || (!touchMode && !live.some(context=>context.state!=='running'));
    audioButton.textContent = live.some(context=>context.state!=='running') ? 'Enable sound' : 'Sound';
  }
  audioButton?.addEventListener('click', () => {
    activateAudio(); window.floppyAssets?.retryFailed();
  });
  document.getElementById('asset-retry')?.addEventListener('click', () => window.floppyAssets?.retryFailed());
  window.floppyAssetStateChanged = () => { if (assetError) assetError.hidden = inlinePresentation || !window.floppyAssets.summary().failed; };
  const jumpPointers = new Set(), shootPointers = new Set();
  function touchAction(action, down) {
    if (started && !disposed) Module.ccall('floppy_touch', null, ['number','number'], [action, down ? 1 : 0]);
  }
  function releaseTouches() {
    jumpPointers.clear(); shootPointers.clear(); touchRoles.clear();
    touchAction(1, false);
    if (started && !disposed) Module.ccall('floppy_reset_input', null, [], []);
  }
  function needsRotation() {
    if (!started || !touchMode || inlinePresentation || !(gameUiState & 1)) return false;
    const portrait = canvas.clientHeight >= canvas.clientWidth;
    return !!((gameUiState & 16) && !portrait || (gameUiState & 32) && portrait);
  }
  function updateMobileControls() {
    if (started && appliedTouchMode !== touchMode) {
      appliedTouchMode = touchMode;
      touchAction(3, touchMode);
    }
    const visible = started && touchMode && !inlinePresentation && state.visible && !document.hidden && !disposed;
    const waiting=needsRotation();
    if (waiting !== orientationWaiting) {
      orientationWaiting=waiting; releaseTouches();
      Module.ccall('floppy_orientation',null,['number'],[waiting ? 1 : 0]);
    }
    if (rotationGate) rotationGate.hidden=!visible || !waiting;
    if (rotationTitle) rotationTitle.textContent=gameUiState & 32 ? 'Turn your phone sideways' : 'Turn your phone upright';
    if (rotationDescription) rotationDescription.textContent=gameUiState & 32 ? 'The final level uses landscape. Your run is paused while you rotate.' : 'Normal levels use portrait. Your run is paused while you rotate.';
    if (chrome) chrome.hidden = !started || inlinePresentation || !state.visible || document.hidden || disposed;
    fullscreen.hidden = embedded || inlinePresentation || (typeof document.documentElement?.requestFullscreen !== 'function');
    const active = !!(gameUiState & 1);
    const paused = !!(gameUiState & 4), gameOver = !!(gameUiState & 8);
    mobileTop.hidden = !visible;
    backButton.hidden = !embedded;
    pauseButton.hidden = !active || gameOver || waiting;
    pauseButton.textContent = paused ? 'Resume' : 'Pause';
    mobileBottom.hidden = !visible || !active || paused || gameOver || waiting;
    shootButton.hidden = !(gameUiState & 2);
    updateAudioControl();
    if (touchMode && !started) document.getElementById('hint').textContent = 'Tap the game or Jump · Hold Shoot when unlocked · Pause anytime';
    if (started) resize();
  }
  window.floppySetUiState = mask => { gameUiState = mask; updateMobileControls(); };
  window.addEventListener('pointerdown', event => {
    if (event.pointerType === 'touch' || event.pointerType === 'pen') { touchMode = true; updateMobileControls(); }
    if (started && !disposed) activateAudio();
  }, {capture:true, passive:true});
  window.addEventListener('pointerup', () => { if (started && !disposed) activateAudio(); }, {capture:true,passive:true});
  function capture(event, element) {
    event.preventDefault(); event.stopImmediatePropagation();
    element.setPointerCapture?.(event.pointerId);
  }
  canvas.addEventListener('pointerdown', event => {
    if (!started || orientationWaiting || !(gameUiState & 1) || (gameUiState & 12) || !['touch','pen'].includes(event.pointerType)) return;
    capture(event, canvas); jumpPointers.add(event.pointerId); touchAction(0, true);
  }, {capture:true, passive:false});
  function endCanvasTouch(event) {
    if (!jumpPointers.delete(event.pointerId)) return;
    event.preventDefault(); event.stopImmediatePropagation(); touchAction(0, false);
  }
  canvas.addEventListener('pointerup', endCanvasTouch, {capture:true, passive:false});
  canvas.addEventListener('pointercancel', endCanvasTouch, {capture:true, passive:false});
  function bindControl(element, action, held = false) {
    element.addEventListener('pointerdown', event => {
      capture(event, element);
      if (held) shootPointers.add(event.pointerId);
      touchAction(action, true);
    }, {passive:false});
    const end = event => {
      event.preventDefault();
      if (held) { shootPointers.delete(event.pointerId); if (shootPointers.size) return; }
      touchAction(action, false);
    };
    element.addEventListener('pointerup', end);
    element.addEventListener('pointercancel', end);
    element.addEventListener('lostpointercapture', end);
    element.addEventListener('click', event => {
      if (event.detail === 0) { touchAction(action, true); touchAction(action, false); }
    });
  }
  bindControl(jumpButton, 0);
  bindControl(shootButton, 1, true);
  bindControl(pauseButton, 2);
  backButton.addEventListener('click', () => send('game:request-close'));
  document.getElementById('rotation-back')?.addEventListener('click', () => touchAction(4,true));
  window.addEventListener('blur', () => { releaseTouches(); applyState(); });
  window.addEventListener('focus', applyState);
  updateMobileControls();
  function send(type, payload = {}) {
    if (embedded && validParent) window.parent.postMessage({ channel: 'floppy-turd', version: 1, session, type, payload }, parentOrigin);
  }
  window.floppySend = type => send(type === 'exit' ? 'game:request-close' : type);
  function resize() {
    if (!started) return;
    if (needsRotation() !== orientationWaiting) { updateMobileControls(); return; }
    const width = inlinePresentation ? 800 : Math.max(1, Math.round(canvas.clientWidth));
    const height = inlinePresentation ? 450 : Math.max(1, Math.round(canvas.clientHeight));
    const style = safeProbe && window.getComputedStyle ? window.getComputedStyle(safeProbe) : {};
    const number = value => Math.max(0, parseFloat(value) || 0);
    const bounds = element => element?.getBoundingClientRect?.();
    const canvasRect = bounds(canvas) || {top:0,bottom:height};
    let top=number(style.paddingTop), right=number(style.paddingRight), bottom=number(style.paddingBottom), left=number(style.paddingLeft);
    if (touchMode && !inlinePresentation) {
      const header = chrome && !chrome.hidden ? bounds(chrome) : null;
      const dock = mobileBottom && !mobileBottom.hidden ? bounds(mobileBottom) : null;
      if (header) top=Math.max(top,header.bottom-canvasRect.top+10);
      if (dock) bottom=Math.max(bottom,canvasRect.bottom-dock.top+10);
    }
    const layout = JSON.stringify([width,height,top,right,bottom,left,touchMode,inlinePresentation]);
    if (layout === lastLayout) return;
    releaseTouches(); lastLayout=layout;
    Module.ccall('floppy_resize', null, ['number','number'], [width, height]);
    Module.ccall('floppy_safe_area', null, ['number','number','number','number'], [top,right,bottom,left]);
  }
  function applyState() {
    updateMobileControls();
    if (!started) return;
    const suspended = disposed || !state.visible || document.hidden || (!embedded && document.hasFocus && !document.hasFocus());
    if (suspended) releaseTouches();
    Module.ccall('floppy_set_paused', null, ['number'], [suspended ? 1 : 0]);
    resize();
  }
  function selectPlaybackRoute() {
    try {
      const session=window.navigator?.audioSession;
      if (session && session.type!=='playback') session.type='playback';
    } catch {}
  }
  function activateAudio() {
    selectPlaybackRoute();
    window.floppyStartMuted = false;
    if (started) Module.ccall('floppy_activate_audio', null, ['number'], [1]);
    return Promise.all([...audioContexts].map(context => {
      if (context.state === 'running' || context.state === 'closed') return true;
      // Safari can report "interrupted" after backgrounding or an audio route change.
      try {
        const resumed=context.resume().then(()=>{updateAudioControl();return context.state==='running';}).catch(()=>false);
        // Starting a silent source from this same gesture also unlocks older WebKit.
        if (context.createBufferSource) {
          const source=context.createBufferSource();
          source.buffer=context.createBuffer(1,1,context.sampleRate);
          source.connect(context.destination); source.start();
          source.onended=()=>source.disconnect();
        }
        return resumed;
      } catch { return false; }
    }));
  }
  function getViewport() {
    const width = canvas.width || 800, height = canvas.height || 450;
    if (touchMode && !inlinePresentation) return {x:0,y:0,width,height};
    let scale = Math.max(0.001, Math.min(width/400,height/225));
    if (!touchMode && scale >= 1) scale = Math.floor(scale);
    return {x:(width-400*scale)/2,y:(height-225*scale)/2,width:400*scale,height:225*scale};
  }
  function setPresentation(value) {
    if (!value || typeof value.inline !== 'boolean') return false;
    releaseTouches(); inlinePresentation = value.inline;
    if (started) Module.ccall('floppy_set_presentation',null,['number'],[inlinePresentation?1:0]);
    fullscreen.hidden = embedded || inlinePresentation;
    resize(); updateMobileControls();
    if (!inlinePresentation) canvas.focus();
    return true;
  }
  const keyCodes = {Space:32,Escape:256,Enter:257,Tab:258,Backspace:259,Insert:260,Delete:261,ArrowRight:262,ArrowLeft:263,ArrowDown:264,ArrowUp:265,Home:268,End:269,ShiftLeft:340,ControlLeft:341,AltLeft:342,MetaLeft:343,ShiftRight:344,ControlRight:345,AltRight:346,MetaRight:347};
  function keyCode(code) {
    if (Object.hasOwn(keyCodes,code)) return keyCodes[code];
    if (/^Key[A-Z]$/.test(code)) return code.charCodeAt(3);
    if (/^Digit[0-9]$/.test(code)) return code.charCodeAt(5);
    if (/^F(?:[1-9]|1[0-2])$/.test(code)) return 289+Number(code.slice(1));
    return null;
  }
  const touchRoles = new Map();
  function input(payload, messageTransport = false) {
    if (!payload || payload.schema !== 'portfolio-input' || payload.version !== 1 || disposed) return false;
    if (payload.kind === 'reset') {
      if (!['blur','hidden','presentation','exit','cancel'].includes(payload.reason)) return false;
      touchRoles.clear(); releaseTouches(); return true;
    }
    if (payload.kind === 'capabilities') {
      if (!Number.isInteger(payload.maxTouchPoints) || payload.maxTouchPoints < 0 || !['fine','coarse','none'].includes(payload.primaryPointer) || typeof payload.anyCoarsePointer !== 'boolean' || typeof payload.hover !== 'boolean') return false;
      touchMode = payload.maxTouchPoints > 0 || payload.primaryPointer === 'coarse' || payload.anyCoarsePointer;
      updateMobileControls(); return true;
    }
    if (!started || orientationWaiting) return false;
    if (payload.kind === 'key') {
      const modifiers = payload.modifiers;
      if (!['down','up'].includes(payload.phase) || typeof payload.repeat !== 'boolean' || !modifiers || !['shift','ctrl','alt','meta'].every(key=>typeof modifiers[key]==='boolean')) return false;
      const code = keyCode(payload.code); if (code === null) return false;
      if (modifiers.ctrl || modifiers.alt || modifiers.meta) return false;
      if (payload.phase === 'down' && !payload.repeat && embedded && ((payload.code === 'Escape' && modifiers.shift) || payload.code === 'KeyF')) {
        send(payload.code === 'Escape' ? 'game:request-close' : 'game:request-fullscreen'); return true;
      }
      if (!messageTransport && payload.phase === 'down') activateAudio();
      Module.ccall('floppy_key',null,['number','number','number'],[code,payload.phase==='down'?1:0,payload.repeat?1:0]);
      return true;
    }
    if (payload.kind === 'pointer') {
      if (!['down','move','up','cancel'].includes(payload.phase) || !Number.isInteger(payload.pointerId) || !['mouse','touch','pen'].includes(payload.pointerType) || !Number.isFinite(payload.x) || payload.x<0 || payload.x>1 || !Number.isFinite(payload.y) || payload.y<0 || payload.y>1 || !Number.isInteger(payload.button) || payload.button < -1 || payload.button > 4 || !Number.isInteger(payload.buttons) || payload.buttons<0 || payload.buttons>31 || !Number.isFinite(payload.pressure) || payload.pressure<0 || payload.pressure>1) return false;
      if (!messageTransport && payload.phase === 'down') activateAudio();
      if (payload.pointerType !== 'mouse') { touchMode=true; updateMobileControls(); }
      if (payload.pointerType !== 'mouse' && (gameUiState & 1) && !(gameUiState & 12)) {
        if (payload.phase === 'down') {
          const role = (gameUiState & 2) && payload.x>=0.65 && payload.y>=0.65 ? 1 : 0;
          touchRoles.set(payload.pointerId,role); touchAction(role,true);
        } else if (payload.phase === 'up' || payload.phase === 'cancel') {
          const role = touchRoles.get(payload.pointerId); touchRoles.delete(payload.pointerId);
          if (role === 1 && ![...touchRoles.values()].includes(1)) touchAction(1,false);
        }
        return true;
      }
      if (payload.button > 0) return false;
      Module.ccall('floppy_pointer',null,['number','number','number'],[{move:0,down:1,up:2,cancel:3}[payload.phase],payload.x,payload.y]);
      return true;
    }
    return false;
  }
  window.floppyEmbed = Object.freeze({version:1,canvas,input:payload=>input(payload),activateAudio,setPresentation,getViewport,
    getAudioState:()=>[...audioContexts].map(context=>({state:context.state,time:context.currentTime,sampleRate:context.sampleRate})),
  });
  window.portfolioGame = window.floppyEmbed;
  if (autostart) gate.hidden = true;
  function fail(error) {
    const message = String(error).slice(0,500);
    status.textContent = 'The game could not load. Please reload to try again.';
    gate.hidden = false;
    play.hidden = true;
    send('game:error', { message });
    console.error(error);
  }
  window.addEventListener('message', event => {
    if (!embedded || !validParent || event.source !== window.parent || event.origin !== parentOrigin) return;
    const data = event.data;
    if (!data || data.channel !== 'floppy-turd' || data.version !== 1 || data.session !== session) return;
    if (data.type === 'host:input') { input(data.payload,true); return; }
    if (data.type === 'host:dispose') { disposed = true; state.visible = false; applyState(); return; }
    if (data.type === 'host:init' || data.type === 'host:state') {
      if (!data.payload || typeof data.payload.visible !== 'boolean') return;
      state = data.payload;
      applyState();
      if (initialized && data.type === 'host:init') send('game:ready');
    }
  });
  window.addEventListener('keydown', event => {
    if (event.ctrlKey || event.metaKey || event.altKey || event.target?.isContentEditable || event.target?.closest?.('input,textarea,select')) return;
    if (event.target?.closest?.('button') && !['Escape','KeyF'].includes(event.code)) return;
    if (embedded && ((event.code === 'Escape' && event.shiftKey) || event.code === 'KeyF')) {
      event.preventDefault(); event.stopImmediatePropagation();
      send(event.code === 'Escape' ? 'game:request-close' : 'game:request-fullscreen');
    } else if (!embedded && event.code === 'KeyF') { event.preventDefault(); requestFullscreen(); }
    else if (started) {
      input({schema:'portfolio-input',version:1,kind:'key',phase:'down',code:event.code,repeat:!!event.repeat,modifiers:{shift:!!event.shiftKey,ctrl:false,alt:false,meta:false}});
      if (['Space','Tab'].includes(event.code)) event.preventDefault();
    }
  }, true);
  window.addEventListener('keyup', event => {
    if (started) input({schema:'portfolio-input',version:1,kind:'key',phase:'up',code:event.code,repeat:false,modifiers:{shift:!!event.shiftKey,ctrl:!!event.ctrlKey,alt:!!event.altKey,meta:!!event.metaKey}},true);
  }, true);
  document.addEventListener('visibilitychange', applyState);
  window.addEventListener('pagehide', () => { disposed = true; applyState(); });
  new ResizeObserver(resize).observe(canvas);
  if (chrome) new ResizeObserver(resize).observe(chrome);
  if (mobileBottom) new ResizeObserver(resize).observe(mobileBottom);
  window.visualViewport?.addEventListener('resize', resize);
  window.Module = {
    canvas,
    noInitialRun: true,
    preRun: [() => {
      FS.mkdirTree('/saves');
      for (const [key,name] of [['floppy-turd-save-v2','floppy_turd_stats.bin'],['floppy-turd-save-v3','floppy_turd_stats_v3.bin']]) {
        try {
          const encoded = localStorage.getItem(key);
          if (encoded) FS.writeFile('/saves/'+name, Uint8Array.from(atob(encoded), char => char.charCodeAt(0)));
        } catch (error) { console.warn('Saved progress unavailable', error); }
      }
    }],
    setStatus: text => { if (!initialized && text) status.textContent = text; },
    onAbort: fail,
    onRuntimeInitialized: () => {
      initialized = true;
      status.textContent = 'Ready to play.';
      play.hidden = autostart;
      updateMobileControls();
      if (autostart) startGame();
      send('game:ready');
    },
  };
  function startGame() {
    if (!initialized || started || disposed) return;
    started = true; gate.hidden = true;
    try {
      Module.callMain([]);
      Module.ccall('floppy_set_presentation',null,['number'],[inlinePresentation?1:0]);
      resize(); applyState();
      if (!inlinePresentation) canvas.focus();
    } catch (error) { fail(error); }
  }
  play.addEventListener('click', () => { selectPlaybackRoute(); startGame(); activateAudio(); });
})();
