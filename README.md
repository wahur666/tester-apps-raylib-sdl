# Touch Ray Demo

This repository builds two native applications:

- `touch_ray_demo`: the multi-touch tracker.
- `webcam_preview`: a webcam preview rendered through raylib using SDL3 camera frames.

No extra webcam library is required. SDL3 provides the camera API and is already
vendored in `SDL-release-3.4.10`; raylib is already vendored in `raylib-6.0`.

The Docker image builds the application on Alpine Linux with musl libc. It only
produces a build artifact; run the application natively on the target system.

## Build a musl binary

Build from the project directory:

```sh
docker build --target export --output type=local,dest=dist .
```

The resulting touch tracker executable is written to `dist/touch_ray_demo`. It
is a Linux musl binary and will not run directly on Windows. It is not fully
static: X11 and OpenGL remain runtime dependencies on the target musl-based
Linux system.

## Build on Windows

Run the PowerShell release build script from the project directory:

```powershell
.\build-windows.ps1
```

Use `-Clean` to remove the previous release build before compiling:

```powershell
.\build-windows.ps1 -Clean
```

The executables are copied to `dist\windows\touch_ray_demo.exe` and
`dist\windows\webcam_preview.exe`. The script uses tools available on `PATH`, or
falls back to CLion's bundled CMake, Ninja, and MinGW toolchain when CLion is
installed in its default per-user location.
