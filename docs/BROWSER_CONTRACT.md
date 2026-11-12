# Floppy Turd browser contract v1

This is the stable interface between revision builds of the game and the 3D portfolio website. Gameplay, renderer, fonts, controls, and native platforms can change without modifying the website scene.

## Distribution and revision identity

Run `python tools/package-web.py` after a successful browser build. The directory `dist/web` contains these **five stable runtime files** and an optional, recommended manifest:

- `index.html` — entry point
- `index.js` — Emscripten runtime
- `index.wasm` — game binary
- `index.data` — resources
- `bridge.js` — child lifecycle/input shell
- `build-manifest.json` — schema below; not included in its own file list

All runtime filenames and their relative layout are part of v1. Replace a whole revision using the website's `npm run stage:floppy-turd -- --from <dist/web>` command, then rebuild the website. Do not copy individual artifacts into a running distribution. The stager verifies a complete candidate before activation and retains the previous snapshot.

Manifest schema:

```json
{
  "schemaVersion": 1,
  "buildId": "git-<revision>-<artifact-digest>",
  "commit": "<40 lowercase hex characters or null for an explicit local build>",
  "protocolVersion": 1,
  "entry": "index.html",
  "files": {
    "index.html": {"bytes": 123, "sha256": "<64 lowercase hex characters>"},
    "index.js": {"bytes": 123, "sha256": "<64 lowercase hex characters>"},
    "index.wasm": {"bytes": 123, "sha256": "<64 lowercase hex characters>"},
    "index.data": {"bytes": 123, "sha256": "<64 lowercase hex characters>"},
    "bridge.js": {"bytes": 123, "sha256": "<64 lowercase hex characters>"}
  }
}
```

`bytes` is a nonnegative integer. All five entries are required; paths cannot escape the bundle. The packager uses `commit:null` when the source checkout is dirty and identifies that build by its artifact digest. Hosts keep accepting earlier five-file builds without a manifest; when supplied, every size/hash and the protocol must validate before activation.

## Hosting and persistence

The stable website path is `/games/floppy-turd/index.html`. Keep the game origin, path, and localStorage key `floppy-turd-save-v2` unchanged across revisions. Do not namespace saves by buildId. A save schema migration is a separate change from a build replacement.

Use HTTP(S), serve `.wasm` as `application/wasm`, and permit same-origin embedding. This baseline is single-threaded and requires no backend or cross-origin isolation. The host lazy-loads the iframe when the user enters the room containing the live TV; opening the portfolio alone must not download the large game data file.

## Ownership

The website owns CRT selection/transition, the iframe, native/full-window fullscreen, host visibility, and returning to the room. The game owns its canvas, aspect ratio/letterboxing, simulation, audio unlock, keyboard/mouse/touch controls, save compatibility, and responsive UI. Changing the game render resolution must not require a website scene change.

The child receives `embed=1`, optional `autostart=1`, `parentOrigin=<exact origin>`, and `session=<per-open nonce>`. Without embed mode, the same entry is independently playable. Optional `touch=1` previews touch controls on a desktop; ordinary detection uses capabilities and actual pointer types, not a device-name list.

## postMessage protocol

Every envelope is `{channel:'floppy-turd',version:1,session,type,payload}`. Validate exact WindowProxy identity, origin, channel, version and current session. Send only to the known exact target origin.

| Direction | Type | Payload |
| --- | --- | --- |
| Host → game | `host:init` | `{visible,fullscreen,viewport:{width,height,dpr}}` |
| Host → game | `host:state` | Same full state snapshot; CSS pixel dimensions, DPR capped by the host |
| Host → game | `host:dispose` | `{reason:'exit'}` |
| Game → host | `game:ready` | `{}` |
| Game → host | `game:request-close` | `{}` |
| Game → host | `game:request-fullscreen` | `{}` |
| Game → host | `game:error` | `{message}`; at most 500 characters |

Install the child listener before WASM startup; queue initial visibility until the game exists. A ready message means the runtime is initialized; autostart boots the game muted without a Play gate, and a real user gesture activates audio. Standalone retains click-to-start. The host sends init on load and the latest state after ready, then on resize/visibility/fullscreen changes. Duplicate ready/init messages are harmless.

Hidden/disposed games freeze simulation and mute audio, clear held touch input, and save progress. Resuming host visibility must preserve the player's own pause state and discard the first elapsed-time step. Disposal/save delivery before iframe destruction is best effort, so the game also saves during normal progression.

F requests host fullscreen; Shift+Escape requests return to the room. Plain Escape and Tab pause the game. Native fullscreen requests can lack user activation; a real host button and a usable full-window fallback are required. Modified browser shortcuts and editable elements pass through. Tab pauses independently of the host.

## Mobile behavior

Use `maxTouchPoints`, coarse-pointer media queries, and actual touch/pen pointer events. Show sufficiently large, safe-area-aware controls when appropriate. During active gameplay, canvas taps or Jump trigger a single jump; holding Shoot repeats through the game's cooldown after the shooting skill is unlocked. Pause/Resume and embedded Back are real DOM buttons. Menu touches continue through the canvas UI. Cancelled pointers, focus loss, hidden state, and disposal release held actions; multiple fingers can jump and shoot independently.

Touch presentation uses the available viewport continuously, while desktop prefers integer enlargement; both use point filtering. The game remains landscape 16:9 and letterboxes in portrait instead of relying on orientation locking. Browser capabilities vary; iOS/Android real-device audio, memory, fullscreen, and sustained performance require device testing. This is web access on those platforms, separate from native App Store/Play Store packaging.

## Compatibility policy and acceptance checks

Preserve envelope version, message semantics, file names, entry point and save key within v1. Add optional fields compatibly; do not change required meaning silently. Breaking transport/layout changes require a new negotiated version and coordinated host update. Gameplay revisions do not bump the transport version.

Each revision must pass: native/save checks; bridge sender/session/lifecycle tests; exact-case asset audit; manifest hash validation; clean browser boot; muted autostart and first-gesture audio activation; jump/shoot/pause controls; host hide/resume with separate player pause; portrait/landscape letterboxing; return/disposal; and a smoke check through the actual website. Clearly distinguish simulated touch testing from physical-device validation.


## Same-origin live presentation API

`window.portfolioGame` is the reusable interface; `window.floppyEmbed` is a compatible alias for this game. Both refer to the same object:

```js
{
  version: 1,
  canvas,                       // drawable HTMLCanvasElement
  getViewport(),                // {x,y,width,height} in canvas bitmap pixels
  input(payload),               // portfolio-input v1; true when accepted
  activateAudio(),              // call synchronously from a genuine host gesture
  setPresentation({inline})     // keep the same running session
}
```

Use `embed=1&autostart=1` for the live CRT. It boots muted without a second Play gate. In inline mode the child renders at 800×450, uses host visibility rather than iframe focus and hides standalone DOM controls. Keep the same-origin iframe alive and opacity-hidden rather than display:none; the host controls visibility and may pause it when the room is not visible. Embedded WebGL requests `preserveDrawingBuffer:true`, so the host can copy its canvas after rendering. `getViewport()` identifies the actual drawable region for texture cropping and UV mapping. Switching to expanded presentation resizes the existing canvas/session instead of creating another game instance.

Do not route input by synthesizing child DOM events. Call `input(payload)` directly from the host's actual pointer/key handler; input uses latched C++ edges, including rapid down/up within a frame. Call `activateAudio()` from that genuine host gesture. Optional `host:input` carries the identical payload through the origin/session-checked envelope, but asynchronous messages cannot promise audio activation. Reset input on blur, hiding, presentation changes and exit.

### Shared game-agnostic input data

The machine-readable schema is `web/portfolio-input.schema.json` (`urn:portfolio-input:v1`). This wire format contains device data, not game-specific actions:

```js
// Pointer coordinates are normalized to the actual game viewport, top-left origin.
{schema:'portfolio-input',version:1,kind:'pointer',phase:'down',
 pointerId:1,pointerType:'touch',x:0.4,y:0.5,button:0,buttons:1,pressure:0.5}

{schema:'portfolio-input',version:1,kind:'key',phase:'down',code:'Space',repeat:false,
 modifiers:{shift:false,ctrl:false,alt:false,meta:false}}

{schema:'portfolio-input',version:1,kind:'reset',reason:'blur'}

{schema:'portfolio-input',version:1,kind:'capabilities',maxTouchPoints:5,
 primaryPointer:'coarse',anyCoarsePointer:true,hover:false}
```

Pointer phases: down/move/up/cancel; types: mouse/touch/pen. Exclude CRT/canvas letterbox margins before normalizing. Keyboard uses physical DOM `code` values. Reset reasons: blur/hidden/presentation/exit/cancel. Preserve independent pointer IDs; cancellation releases state without clicking a menu item. A game's adapter decides what those inputs mean. Floppy Turd maps gameplay touch downs to jumping and its bottom-right shooting region to held shooting when unlocked; menus use the same normalized pointer data. Future games can interpret the same schema differently.

Fields can be extended compatibly within v1. Required meaning, coordinate spaces and phases must not change silently. Keep transport/input schema versions separate from game build IDs.
