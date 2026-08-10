#!/bin/bash

# Buscamos si el proceso ya existe
if pgrep -x "zmenun223" > /dev/null
then
    # Si existe, lo matamos (ocultar)
    pkill -x "zmenun223"
    pkill -x "zmetrics-server"  # Intentar matar el proceso nuevamente para asegurarse
else
    # Si no existe, entramos a la carpeta y lo lanzamos (mostrar)
    cd /home/alb/zaramaga-static/projects/zmenu && GSK_RENDERER=cairo WAYLAND_DISPLAY=wayland-1 ./zmenun223 400 768 >/dev/null 2>&1 &
fi
