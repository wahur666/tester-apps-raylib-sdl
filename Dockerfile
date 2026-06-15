FROM alpine:3.22 AS builder

RUN apk add --no-cache \
        build-base \
        cmake \
        ninja \
        pkgconf \
        linux-headers \
        mesa-dev \
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

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG" \
        -DSDL_AUDIO=OFF \
        -DSDL_CAMERA=OFF \
        -DSDL_DBUS=OFF \
        -DSDL_HAPTIC=OFF \
        -DSDL_JOYSTICK=OFF \
        -DSDL_LIBUDEV=OFF \
        -DSDL_SENSOR=OFF \
        -DSDL_WAYLAND=OFF \
        -DSDL_X11=ON \
    && cmake --build build --target touch_ray_demo --parallel

FROM scratch AS export
COPY --from=builder /src/build/touch_ray_demo /touch_ray_demo
