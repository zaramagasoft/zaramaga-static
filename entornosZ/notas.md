1. El archivo está en /usr/bin/ y no en la ruta local del usuario

Cuando lanzas Steam sin privilegios de root (como usuario normal), el cliente prefiere o busca primero el script en el PATH local del usuario (~/.local/bin/). Si no lo encuentra ahí con permisos de ejecución, lo ignora o falla la llamada D-Bus.

Crea el archivo directamente en la carpeta local del usuario:
Bash

mkdir -p ~/.local/bin
nano ~/.local/bin/steamos-session-select

2. Lo que debe contener el script

Si lo que quieres es que al pulsar "Cambiar a escritorio" Steam simplemente se cierre ordenadamente (para matar la ventana de Gamescope) o que ejecute algo como cerrar sesión sin tumbar Sway, el contenido típico del script es:

Opción A: Para que cierre Steam de forma limpia y destruya Gamescope (volviendo a Sway):
Bash

#!/bin/bash
steam -shutdown

Opción B: Para que no haga absolutamente nada si lo pulsas por error:
Bash

#!/bin/bash
exit 0

Opción C: Si quieres que abra un programa en pantalla (ej. una consola):
Bash

#!/bin/bash
foot &

3. Dar permisos de ejecución (Paso imprescindible)

Dale permisos de ejecución al script creado en la carpeta del usuario:
Bash

chmod +x ~/.local/bin/steamos-session-select

4. Asegurarte de que Steam pasa por esa ruta

Para asegurarte de que cuando lanzas el atajo desde Sway se encuentra la carpeta ~/.local/bin, añade PATH a la línea de tu bindsym:
Plaintext

bindsym $mod+Shift+g exec PATH="$HOME/.local/bin:$PATH" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" WAYLAND_DISPLAY=wayland-1 gamescope -W 1920 -H 1080 -w 1920 -h 1080 -r 160 -f -e --mangoapp -- steam -steamos3 -gamepadui > /tmp/gamescopeALB.log 2>&1

Haciendo esto en ~/.local/bin/steamos-session-select, Steam detectará el ejecutable personalizado exactamente como lo tenías configurado en el otro sistema.