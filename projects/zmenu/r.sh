#!/bin/bash

export XDG_RUNTIME_DIR=/run/user/1000

echo "PADRE:"
echo "XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR"
echo "WAYLAND_DISPLAY=$WAYLAND_DISPLAY"

gamescope \
    --expose-wayland \
    -w 800 -h 600 \
    -W 800 -H 600 \
    -- \
    bash -c '
        echo "HIJO:"
        echo "XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR"
        echo "WAYLAND_DISPLAY=$WAYLAND_DISPLAY"
        echo "--- SOCKETS ---"
        ls -la "$XDG_RUNTIME_DIR"/gamescope-* 2>&1
        echo "--- ZDXG ---"
        ./zmenun223 400 500
    ' > /tmp/zxdg_output.log 2>&1

# Después de ejecutar, mira el log:
# cat /tmp/zxdg_output.log