FROM alpine:3.22 AS builder

RUN apk add --no-cache \
        build-base \
        cmake \
        git \
        ninja \
        pkgconf \
        linux-headers \
        mesa-dev \
        wayland-dev \
        wayland-protocols \
        libdecor-dev \
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
        -DSDL_AUDIO=OFF \
        -DSDL_CAMERA=ON \
        -DSDL_DBUS=OFF \
        -DSDL_HAPTIC=OFF \
        -DSDL_JOYSTICK=OFF \
        -DSDL_LIBUDEV=OFF \
        -DSDL_SENSOR=OFF \
        -DSDL_WAYLAND=ON \
        -DSDL_X11=ON \
    && cmake --build build --target touch_ray_demo webcam_preview --parallel

FROM scratch AS export
COPY --from=builder /src/build/touch_ray_demo /touch_ray_demo
COPY --from=builder /src/build/webcam_preview /webcam_preview
