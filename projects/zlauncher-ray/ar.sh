#!/bin/bash
export DRM_DEVICE="/dev/dri/card1"
export DRM_FIRST_CONNECTOR="HDMI-A-2"
# 1. Define aquí el puerto exacto donde quieres forzar tu app (ej: HDMI-A-1, DP-1, DP-2)
PUERTO_DESEADO="HDMI-A-2"
sleep 4
# 2. Comprobación de seguridad: Ver si el puerto realmente existe y está conectado
if ls /sys/class/drm/card*-${PUERTO_DESEADO}/status >/dev/null 2>&1; then
    ESTADO=$(cat /sys/class/drm/card*-${PUERTO_DESEADO}/status)
    if [ "$ESTADO" = "connected" ]; then
        echo "[OK] Forzando el inicio en el puerto: $PUERTO_DESEADO"
    else
        echo "[AVISO] El puerto $PUERTO_DESEADO está disponible pero no tiene un cable conectado."
    fi
else
    echo "[ERROR] El puerto '$PUERTO_DESEADO' no existe en tu tarjeta gráfica."
    echo "Los puertos disponibles en tu sistema son:"
    ls /sys/class/drm/ | grep -E "HDMI|DP|DVI" | sed 's/card1-//'
    exit 1
fi

# 3. Exportar la variable de entorno nativa de DRM
export DRM_FIRST_CONNECTOR="$PUERTO_DESEADO"
sleep 5
# 4. Lanzar tu aplicación DRM (Modifica la ruta si tu binario está en otra carpeta)
./zlauncher-rayL
