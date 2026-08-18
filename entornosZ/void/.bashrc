# .bashrc

# If not running interactively, don't do anything
[[ $- != *i* ]] && return

alias ls='ls --color=auto'
PS1='[\u@\h \W]\$ '
#
# ~/.bashrc
#

alias muslcc='x86_64-linux-musl-gcc -static'

PS1='[\u@\h \W]\$ '

export LIBVA_DRIVER_NAME=nvidia
export NVD_BACKEND=direct
export MOZ_DISABLE_RDD_SANDBOX=1
export MOZ_ENABLE_WAYLAND=1

export PATH="$HOME/.local/bin:$PATH"

export ZROOT="$HOME/zaramaga-static"
export ZSRC="$ZROOT/src"
export ZBUILD="$ZROOT/build"
export ZSYS="$ZROOT/install"
export ZRECIPES="$ZROOT/recipes"
export ZDOWNLOADS="$ZROOT/downloads"

export CC="$ZSYS/bin/musl-gcc"
export AR=ar
export RANLIB=ranlib
export STRIP=strip
export PKG_CONFIG_PATH="$ZSYS/lib/pkgconfig"
