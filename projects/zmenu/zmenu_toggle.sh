#!/bin/bash

# Buscamos si el proceso ya existe
if pgrep -x "zmenun223" > /dev/null
then
    # Si existe, lo matamos (ocultar)
    pkill -x "zmenun223"
    pkill -x "zmetrics-server"  # Intentar matar el proceso nuevamente para asegurarse
else
    cd /home/alb/zaramaga-static/projects/zmenu &&
    XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    WAYLAND_DISPLAY="$WAYLAND_DISPLAY" \
    GSK_RENDERER=cairo \
    ./zmenun223 400 768 >/tmp/zmenun223.log &
fi
