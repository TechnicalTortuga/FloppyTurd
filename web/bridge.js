(() => {
  'use strict';
  const query = new URLSearchParams(location.search);
  const embedded = query.get('embed') === '1' && window.parent !== window;
  const session = query.get('session') || '';
  const parentOrigin = query.get('parentOrigin') || '';
  const validParent = (() => { try { return new URL(parentOrigin).origin === parentOrigin; } catch { return false; } })();
  const canvas = document.getElementById('canvas');
  const gate = document.getElementById('gate');
  const play = document.getElementById('play');
  const fullscreen = document.getElementById('fullscreen');
  function requestFullscreen() {
    if (document.fullscreenElement) document.exitFullscreen?.().catch(console.warn);
    else document.documentElement.requestFullscreen?.().catch(() => { status.textContent = 'Native fullscreen is unavailable in this browser. The game still fits this window.'; });
  }
  fullscreen.hidden = embedded;
  fullscreen.addEventListener('click', requestFullscreen);
  const status = document.getElementById('status');
  document.getElementById('hint').textContent = embedded ? 'Space: jump · S: shoot · Tab: pause · Esc: return to website' : 'Space: jump · S: shoot · Tab / Esc: pause';
  let initialized = false;
  let started = false;
  let disposed = false;
  let state = { visible: !embedded };
  function send(type, payload = {}) {
    if (embedded && validParent) window.parent.postMessage({ channel: 'floppy-turd', version: 1, session, type, payload }, parentOrigin);
  }
  window.floppySend = type => send(type === 'exit' ? 'game:request-close' : type);
  function resize() {
    if (!started) return;
    const width = Math.max(1, Math.round(canvas.clientWidth));
    const height = Math.max(1, Math.round(canvas.clientHeight));
    Module.ccall('floppy_resize', null, ['number','number'], [width, height]);
  }
  function applyState() {
    if (!started) return;
    Module.ccall('floppy_set_paused', null, ['number'], [disposed || !state.visible || document.hidden ? 1 : 0]);
    resize();
  }
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
    if (embedded && (event.code === 'Escape' || event.code === 'KeyF')) {
      event.preventDefault(); event.stopImmediatePropagation();
      send(event.code === 'Escape' ? 'game:request-close' : 'game:request-fullscreen');
    } else if (!embedded && event.code === 'KeyF') { event.preventDefault(); requestFullscreen(); }
    else if (started && ['Space','Tab'].includes(event.code)) event.preventDefault();
  }, true);
  document.addEventListener('visibilitychange', applyState);
  window.addEventListener('pagehide', () => { disposed = true; applyState(); });
  new ResizeObserver(resize).observe(canvas);
  window.Module = {
    canvas,
    noInitialRun: true,
    preRun: [() => {
      FS.mkdirTree('/saves');
      try {
        const encoded = localStorage.getItem('floppy-turd-save-v2');
        if (encoded) FS.writeFile('/saves/floppy_turd_stats.bin', Uint8Array.from(atob(encoded), char => char.charCodeAt(0)));
      } catch (error) { console.warn('Saved progress unavailable', error); }
    }],
    setStatus: text => { if (!initialized && text) status.textContent = text; },
    onAbort: fail,
    onRuntimeInitialized: () => {
      initialized = true;
      status.textContent = 'Ready to play.';
      play.hidden = false;
      send('game:ready');
    },
  };
  play.addEventListener('click', () => {
    if (!initialized || started || disposed) return;
    started = true;
    gate.hidden = true;
    try { Module.callMain([]); resize(); applyState(); canvas.focus(); }
    catch (error) { fail(error); }
  });
})();
