cambiamos a coid
gamescope -W 1920 -H 1080 -w 1920 -h 1080 -r 60 -f -e --mangoapp -- steam -steamos3 -gamepadui
// raylib rsgl //
make clean
  496  make TARGET_PLATFORM=PLATFORM_DRM      GRAPHICS=GRAPHICS_API_OPENGL_SOFTWARE      CC=musl-gcc      CFLAGS="-O2 -DPLATFORM_DRM -DGRAPHICS_API_OPENGL_SOFTWARE -I$ZSYS/include"      V=1
  497  musl-gcc -static -O2     test_drm.c     -I.     -L.     -L"$ZSYS/lib"     -Wl,--start-group     -lraylib     -ldrm     -lpthread     -lrt     -lm     -ldl     -Wl,--end-group     -o test_drm
  498  file test_drm
//necesario con muslc
sudo pacman -S \
    cmake \
    meson \
    ninja \
    pkgconf \
    bison \
    flex \
    gperf
////en .bashrc////
export PATH="$HOME/.local/bin:$PATH"
export ZROOT=$HOME/zaramaga-static
export ZSRC=$ZROOT/src
export ZBUILD=$ZROOT/build
export ZSYS=$ZROOT/install
export ZRECIPES=$ZROOT/recipes
export ZDOWNLOADS=$ZROOT/downloads