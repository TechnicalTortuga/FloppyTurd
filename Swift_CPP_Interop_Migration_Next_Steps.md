# Swift/C++ Interop Migration: Next Steps and Prompt

## Context
- The project is migrating from legacy C wrappers (e.g., CppInteropBridge, AudioManagerBridge) to **modern Swift/C++ interoperability** using Swift 5.9+ features.
- All C wrappers and bridge files have been removed from both Swift and C++ code.
- The `module.modulemap` in `FloppyTurd/FloppyTurd/` defines the C++ module as `GameEngineCpp`, exposing all C++ headers to Swift.
- Only one Swift file (`MetalRendererSwiftNative.swift`) currently uses `import GameEngineCpp`; the rest are ready to be updated.
- The Xcode build settings have **not yet been updated** for C++ interop and C++20.

---

## What Needs to Be Done

### 1. Update Xcode Build Settings for Swift/C++ Interop
- Select your Swift target in Xcode.
- Go to **Build Settings**.
- Set the following:
  - **Other Swift Flags:**
    ```
    -cxx-interoperability-mode=default -I$(SRCROOT)/FloppyTurd -Xcc -std=c++20
    ```
  - **Import/Search Paths:**
    - Add `$(SRCROOT)/FloppyTurd` to the import/include paths.
  - (Optional) For C++ targets, set **C++ Language Dialect** to `C++20`.

### 2. Update Swift Code to Use Direct C++ Interop
- In every Swift file that needs C++ APIs, add:
  ```swift
  import GameEngineCpp
  ```
- Replace any usage of old C wrappers or `CppInteropBridge` with direct calls to C++ types/functions as exposed by the module.

### 3. Clean and Rebuild
- Clean the build folder (Shift+Cmd+K in Xcode).
- Rebuild the project.

### 4. Debug Any New Errors
- If you encounter build errors, copy the **full error output**.
- Paste the error log into the next chat for further debugging and migration.

---

## Prompt for the Next Agent

> I have removed all C wrappers and bridge files, and my `module.modulemap` defines the C++ module as `GameEngineCpp`. I have not yet updated the Xcode build settings for Swift/C++ interop. Please:
> 1. Apply the correct build settings for Swift/C++ interop and C++20 (see above).
> 2. Update Swift files to use `import GameEngineCpp` and call C++ APIs directly.
> 3. Clean and rebuild the project.
> 4. If there are any build errors, debug and iterate until Swift/C++ interop is working robustly.
> 
> Here is the error output (if any):
> [Paste error log here]

---

**This markdown file provides all the context and a clear, actionable prompt for the next agent to continue the migration efficiently.**
