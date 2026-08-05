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