# Touch Ray Demo

This repository builds two native applications:

- `touch_ray_demo`: the multi-touch tracker.
- `webcam_preview`: a webcam preview rendered through raylib using SDL3 camera frames.

No extra webcam library is required. SDL3 provides the camera API. The project
expects SDL and raylib to be present in `SDL-release-3.4.10` and `raylib-6.0`.

## Install dependencies

Fetch the pinned GitHub dependencies into the folders expected by CMake.

On Windows PowerShell:

```powershell
.\deps.ps1
```

On Linux, macOS, or WSL:

```sh
sh ./deps.sh
```

If the folders already exist, the scripts leave them alone. To replace them with
fresh clones, use the force option for your host:

```powershell
.\deps.ps1 -Force
```

```sh
sh ./deps.sh --force
```

Other available commands:

```powershell
.\deps.ps1 status
.\deps.ps1 clean
```

```sh
sh ./deps.sh status
sh ./deps.sh clean
```

The Docker image builds the application on Alpine Linux with musl libc. It only
produces a build artifact; run the application natively on the target system.
The Docker build also runs `deps.sh`, so a fresh clone can be built directly as
long as Docker has network access during the build.

## Build musl binaries

Build from the project directory:

```sh
docker build --target export --output type=local,dest=dist .
```

The resulting executables are written to `dist/touch_ray_demo` and
`dist/webcam_preview`. They are Linux musl binaries and will not run directly on
Windows. They are not fully static: X11 or Wayland, plus OpenGL/EGL, remain
runtime dependencies on the target musl-based Linux system.

The Docker build enables both SDL video backends:

- X11, for classic X sessions and XWayland.
- Wayland, for native Wayland sessions.

SDL selects the video driver at runtime. To force one while testing on the
target machine:

```sh
SDL_VIDEODRIVER=x11 ./touch_ray_demo
SDL_VIDEODRIVER=wayland ./touch_ray_demo
```

The build does not detect the Docker host display server, because Docker builds
can run on a different machine than the final runtime target.

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
