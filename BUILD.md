# Building Floppy Turd

## Dependencies

CMake 3.20+, a C++20 compiler, Git, and raylib 6.2. CMake fetches the exact commit in `dependencies.lock.json`. Python is needed for the optional asset audit/packaging scripts; Node is needed for browser bridge checks.

On Debian/Ubuntu, install build-essential, cmake, libasound2-dev, libx11-dev, libxrandr-dev, libxi-dev, libxcursor-dev, libxinerama-dev, and libgl1-mesa-dev. Use `-DGLFW_BUILD_WAYLAND=OFF` for the X11 baseline.

## Windows, Linux, and macOS

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --config Release --parallel
ctest --test-dir build/native -C Release --output-on-failure
python tools/audit-assets.py
node --test tests/*.test.mjs
cmake --install build/native --config Release --prefix dist/native
cpack --config build/native/CPackConfig.cmake -C Release -B dist
```

Use `-G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++` for MinGW on Windows. Its game binary links the compiler runtime statically. Keep the executable and adjacent `resources/` directory together. The working directory is resolved from the executable, including when launched from another folder.

The `.github/workflows/build.yml` matrix builds and packages Windows, Linux, macOS, and Web. The initial restoration matrix passed Windows, Linux, macOS, and Web on GitHub Actions (run 43124791254). Subsequent revisions run the same matrix. Artifacts are unsigned. Native Linux/macOS gameplay/audio still requires physical runtime checks.

## Browser

Install and activate the official Emscripten SDK **4.0.23**, then load its environment (`emsdk_env.bat` on Windows, `source emsdk_env.sh` on Unix):

```sh
emcmake cmake -S . -B build/web -DCMAKE_BUILD_TYPE=Release -DFLOPPY_BUILD_TESTS=OFF
cmake --build build/web --parallel
python tools/package-web.py
python tools/serve-web.py --port 8765 --directory dist/web
```

Open `http://127.0.0.1:8765/index.html`; click **Play Floppy Turd** to initialize audio and the game from a real user gesture. The five core files are `index.html`, `index.js`, `index.wasm`, `index.data`, and `bridge.js`. Manifest schema 2 additionally includes `deferred-assets.json` and content-addressed `assets/` blobs; distribute the entire verified directory, not just the core files. See `docs/BROWSER_CONTRACT.md`. Serve `.wasm` as `application/wasm`. No backend, threads, SharedArrayBuffer, or cross-origin isolation headers are required.

For this workspace's local SDK, `python .local/deps/emsdk/upstream/emscripten/emcmake.py cmake ...` also works. Keep temporary compiler files in a writable directory when sandboxing; the local build used `.local/temp`. Local dependency source overrides are optional and are not needed in CI.

The initial core is about 2.2 MB, including an 806,904-byte resource bundle. About 123 MB of music and scenery stays on the server until requested. Only the current level is constructed; its visual dependencies finish loading before simulation starts. Music downloads asynchronously when played and releases its stream/MEMFS bytes when stopped. A 64 MB IndexedDB LRU cache reuses verified compressed assets across refreshes; HTTP caching is the fallback when storage is blocked or evicted. This is caching of downloaded assets, not an offline install of the entire game.

The preview server sends `no-cache` for core files (conditional revalidation) and a one-year immutable cache header for hash-named assets. Production hosting should use the same policy. Keep previously published release directories for already-open sessions.

## Test from a phone

Connect the phone and computer to the same Wi-Fi. Find the computer's Wi-Fi IPv4 address with `ipconfig` on Windows, then run:

```sh
python tools/serve-web.py --bind <computer-Wi-Fi-IP> --port 8766 --directory dist/web
```

Open `http://<computer-Wi-Fi-IP>:8766/index.html` in Safari or Chrome. Keep the computer/server running. `localhost` on the phone refers to the phone itself. This serves the packaged game only; no public hosting or router port forwarding is needed. A firewall or guest-network isolation can prevent other devices connecting.

Play selects Safari's playback audio route and resumes the audio context from that direct tap. Sound provides a recovery action after interruption. DOM control focus is separate from page visibility, so focusing Jump/Pause/Sound does not black out or mute the game.

## Portfolio website integration

The website chat `Plan website 3D scene design` owns `C:/Development/myWebsite`. Its agreed entry point is `/games/floppy-turd/index.html`, loaded lazily in a persistent iframe when the games room is entered. The website owns the fullscreen/full-window player; the game owns its canvas, aspect ratio, pointer mapping, audio and simulation. The 3D scene pauses only while the game is expanded. The same-origin `window.portfolioGame` / `window.floppyEmbed` interface supplies a drawable live canvas, normalized input and presentation switching, so fullscreen expands the existing session.

The website's staging command is:

```sh
npm run stage:floppy-turd -- --from C:/Development/FloppyTurd/dist/web
```

Run it from the website repository. It validates and hashes the clean runtime bundle and preserves the previous local snapshot. The website config points its Floppy Turd entry to `/games/floppy-turd/index.html` after successful staging. Neither repository is deployed by this process. The restoration branch is pushed solely for its build jobs.

Child URL parameters: `embed=1`, optional `autostart=1`, `parentOrigin=<exact origin>`, `session=<nonce>`.

Every message is `{channel:'floppy-turd', version:1, session, type, payload}`. Validate the exact parent/iframe WindowProxy, origin, channel, version and session on both sides; never use a wildcard target origin.

- Host: `host:init`, `host:state` with `{visible, fullscreen, viewport:{width,height,dpr}}`; `host:dispose` with `{reason:'exit'}`.
- Game: `game:ready`, `game:request-close`, `game:request-fullscreen`, and `game:error` with `{message}`.

The shell queues visibility before startup. Hidden/disposed games mute audio and freeze simulation without overwriting the in-game pause flag. The resumed frame is discarded to prevent a large physics time step. Closing the iframe sends a last save request on a best-effort basis. Ctrl/Meta/Alt shortcuts and editable fields pass through. The host supplies an explicit fullscreen retry button if a message-triggered native request lacks user activation.

## Progress files

Windows: `%APPDATA%/FloppyTurd/floppy_turd_stats.bin`; Linux: `$XDG_DATA_HOME/FloppyTurd/` or `~/.local/share/FloppyTurd/`; macOS: `~/Library/Application Support/FloppyTurd/`. Browser: localStorage key `floppy-turd-save-v2`, scoped to the game origin. `FLOPPY_TURD_SAVE_DIR` overrides desktop storage for isolated checks.

Format v2 is 79 bytes, explicitly little-endian counters and one-byte flags, with no C++ struct padding. Loading accepts the old 80-byte v1 Windows/macOS format and rejects truncated/invalid values atomically. To migrate an old portable save, copy it into the new per-user directory before starting. Bundled developer save files are excluded from distributions. This desktop format is separate from the iOS JSON v2 schema.

## Mobile browser controls

Touch/pen/coarse-pointer capabilities enable the browser shell controls without requiring an iOS or Android native build. Use canvas taps or Jump, held Shoot after skill unlock, Pause/Resume, and embedded Back. Controls respect safe areas, clear held actions on cancellation/backgrounding, and support simultaneous jump/shoot pointers. `?touch=1` enables a desktop preview. Portrait keeps landscape letterboxing; real-device browser memory/audio/fullscreen/performance checks remain necessary.

The current same-origin TV adapter and reusable game-agnostic input schema are specified in `docs/BROWSER_CONTRACT.md` and `web/portfolio-input.schema.json`. Plain Escape pauses; Shift+Escape returns to the room. See the contract for live canvas capture, muted autostart and direct input without synthetic DOM events.
