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

## Build Linux binaries

The Alpine/musl build is the most self-contained option. It links the final
executables statically, uses raylib's software renderer to avoid a runtime
OpenGL/Mesa dependency, and asks SDL to link X11/Wayland support instead of
loading those dependency libraries dynamically.

Build from the project directory:

```sh
docker build --target export --output type=local,dest=dist/linux-musl .
```

The resulting executables are written to `dist/linux-musl/touch_ray_demo` and
`dist/linux-musl/webcam_preview`. They are Linux musl binaries and will not run
directly on Windows. The Docker build fails if `ldd` can inspect them as dynamic
executables, so there should be no runtime dependency on the musl loader.

The static musl build still needs a compatible running display server at
runtime. It can talk to X11/XWayland or Wayland, but it cannot create a display
session by itself.

If the static musl build does not work on the target display stack, build the
glibc fallback. It is built on Debian 11, which uses glibc 2.31, matching Ubuntu
20.04 and older than Ubuntu 22.04/24.04. Binaries built against an older glibc
usually run on newer glibc systems, but this variant is less self-contained than
the musl build.

```sh
docker build -f Dockerfile.glibc --target export --output type=local,dest=dist/linux-glibc .
```

The resulting executables are written to `dist/linux-glibc/touch_ray_demo` and
`dist/linux-glibc/webcam_preview`.

Both Docker builds enable both SDL video backends:

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

Chrome and ffmpeg on the target machine are good signs, but they do not
guarantee every runtime library this project needs. If a binary fails to start,
check missing dynamic libraries with:

```sh
ldd ./touch_ray_demo
ldd ./webcam_preview
```

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
