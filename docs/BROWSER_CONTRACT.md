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

Use HTTP(S), serve `.wasm` as `application/wasm`, and permit same-origin embedding. This baseline is single-threaded and requires no backend or cross-origin isolation. The host lazy-loads the iframe only when the user opens the player; opening the portfolio alone must not download the large game data file.

## Ownership

The website owns CRT selection/transition, the iframe, native/full-window fullscreen, host visibility, and returning to the room. The game owns its canvas, aspect ratio/letterboxing, simulation, audio unlock, keyboard/mouse/touch controls, save compatibility, and responsive UI. Changing the game render resolution must not require a website scene change.

The child receives `embed=1`, `parentOrigin=<exact origin>`, and `session=<per-open nonce>`. Without embed mode, the same entry is independently playable. Optional `touch=1` previews touch controls on a desktop; ordinary detection uses capabilities and actual pointer types, not a device-name list.

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

Install the child listener before WASM startup; queue initial visibility until the game exists. A ready message means the runtime is initialized; a separate real user gesture starts the game/audio. The host sends init on load and the latest state after ready, then on resize/visibility/fullscreen changes. Duplicate ready/init messages are harmless.

Hidden/disposed games freeze simulation and mute audio, clear held touch input, and save progress. Resuming host visibility must preserve the player's own pause state and discard the first elapsed-time step. Disposal/save delivery before iframe destruction is best effort, so the game also saves during normal progression.

F requests host fullscreen and Escape requests close while embedded. Native fullscreen requests can lack user activation; a real host button and a usable full-window fallback are required. Modified browser shortcuts and editable elements pass through. Tab pauses independently of the host.

## Mobile behavior

Use `maxTouchPoints`, coarse-pointer media queries, and actual touch/pen pointer events. Show sufficiently large, safe-area-aware controls when appropriate. During active gameplay, canvas taps or Jump trigger a single jump; holding Shoot repeats through the game's cooldown after the shooting skill is unlocked. Pause/Resume and embedded Back are real DOM buttons. Menu touches continue through the canvas UI. Cancelled pointers, focus loss, hidden state, and disposal release held actions; multiple fingers can jump and shoot independently.

Touch presentation uses the available viewport continuously, while desktop prefers integer enlargement; both use point filtering. The game remains landscape 16:9 and letterboxes in portrait instead of relying on orientation locking. Browser capabilities vary; iOS/Android real-device audio, memory, fullscreen, and sustained performance require device testing. This is web access on those platforms, separate from native App Store/Play Store packaging.

## Compatibility policy and acceptance checks

Preserve envelope version, message semantics, file names, entry point and save key within v1. Add optional fields compatibly; do not change required meaning silently. Breaking transport/layout changes require a new negotiated version and coordinated host update. Gameplay revisions do not bump the transport version.

Each revision must pass: native/save checks; bridge sender/session/lifecycle tests; exact-case asset audit; manifest hash validation; clean browser boot; first-gesture start; jump/shoot/pause controls; host hide/resume with separate player pause; portrait/landscape letterboxing; return/disposal; and a smoke check through the actual website. Clearly distinguish simulated touch testing from physical-device validation.
