#!/bin/bash

ZMENU_DIR="/home/alb/zaramaga-static/projects/zmenu"
SOCKET="$XDG_RUNTIME_DIR/zmenu.sock"

if pgrep -x "zmenun223" > /dev/null
then
    # ZMenu ya está arrancado → mandar toggle
    if [ -S "$SOCKET" ]; then
        printf 'TOGGLE\n' | socat - UNIX-CONNECT:"$SOCKET"
    fi
else
    # ZMenu no está arrancado → iniciarlo
    cd "$ZMENU_DIR" || exit 1

    XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    WAYLAND_DISPLAY="$WAYLAND_DISPLAY" \
    GSK_RENDERER=cairo \
    ./zmenun223 400 768 >/tmp/zmenun223.log 2>&1 &
fi