FROM alpine:3.22 AS builder

RUN apk add --no-cache \
        build-base \
        cmake \
        file \
        git \
        ninja \
        pkgconf \
        linux-headers \
        wayland-dev \
        wayland-protocols \
        libxkbcommon-dev \
        libx11-dev \
        libxext-dev \
        libxcursor-dev \
        libxi-dev \
        libxfixes-dev \
        libxrandr-dev \
        libxrender-dev \
        libxscrnsaver-dev \
        libxtst-dev

WORKDIR /src
COPY . .

RUN sh ./deps.sh

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG" \
        -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc" \
        -DOPENGL_VERSION=Software \
        -DSDL_AUDIO=OFF \
        -DSDL_CAMERA=ON \
        -DSDL_DBUS=OFF \
        -DSDL_DEPS_SHARED=OFF \
        -DSDL_HAPTIC=OFF \
        -DSDL_JOYSTICK=OFF \
        -DSDL_LIBUDEV=OFF \
        -DSDL_SENSOR=OFF \
        -DSDL_WAYLAND=ON \
        -DSDL_WAYLAND_LIBDECOR=OFF \
        -DSDL_WAYLAND_SHARED=OFF \
        -DSDL_X11=ON \
        -DSDL_X11_SHARED=OFF \
    && cmake --build build --target touch_ray_demo webcam_preview touch_canvas touch_accuracy --parallel \
    && file build/touch_ray_demo build/webcam_preview build/touch_canvas build/touch_accuracy \
    && ! ldd build/touch_ray_demo \
    && ! ldd build/webcam_preview \
    && ! ldd build/touch_canvas \
    && ! ldd build/touch_accuracy

FROM scratch AS export
COPY --from=builder /src/build/touch_ray_demo /touch_ray_demo
COPY --from=builder /src/build/webcam_preview /webcam_preview
COPY --from=builder /src/build/touch_canvas /touch_canvas
COPY --from=builder /src/build/touch_accuracy /touch_accuracy
