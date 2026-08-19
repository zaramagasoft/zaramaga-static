////////////lanzada de gamescope//////////vez zmenu_toggle.sh
XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" WAYLAND_DISPLAY=wayland-1 gamescope -W 1920 -H 1080 -w 1920 -h 1080 -r 160 -f -e --mangoapp -- steam -steamos3 -gamepadui
/////////////////////////////////////////
1. Crea el archivo de regla
sudo sh -c 'printf "%s\n" "alb ALL=(root) NOPASSWD: /usr/local/libexec/zmenu-bore" > /etc/sudoers.d/zmenu-bore'
2. Dale los permisos correctos
sudo chmod 440 /etc/sudoers.d/zmenu-bore
3. Comprueba que la configuración de sudo es válida
sudo visudo -c

Debe salir algo parecido a:

/etc/sudoers: parsed OK
/etc/sudoers.d/zmenu-bore: parsed OK
4. Ahora prueba el helper
sudo -n /usr/local/libexec/zmenu-bore sched_bore 1
///////////////ojo para lanzar zmenu on///////////
sudo visudo
alb ALL=(root) NOPASSWD: /usr/local/libexec/zmenu-bore
////////////
1. Crea el archivo de regla
sudo sh -c 'printf "%s\n" "alb ALL=(root) NOPASSWD: /usr/local/libexec/zmenu-bore" > /etc/sudoers.d/zmenu-bore'
2. Dale los permisos correctos
sudo chmod 440 /etc/sudoers.d/zmenu-bore
3. Comprueba que la configuración de sudo es válida
sudo visudo -c

////////////////
XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" WAYLAND_DISPLAY="$WAYLAND_DISPLAY" ./zmenun223 400 760
sudo mkdir -p /usr/local/libexec
sudo install -m 755 zmenu-bore /usr/local/libexec/zmenu-bore
 $ZSYS/bin/musl-gcc -static zmenu-bore.c -o zmenu-bore
//////////////////testeo/////////////////
sudo     XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR"     WAYLAND_DISPLAY="$WAYLAND_DISPLAY"     ./zmenun223 400 760

////////////////////////con bore ///////////////////////
$ZSYS/bin/musl-gcc -static -march=native -O2 mainN23.c bore.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun223 $(pkg-config --cflags --libs wayland-client cairo cairo-ft freetype2) -lz -lm -lpthread -s
///////////////////////////////////////////////////////
$ZSYS/bin/musl-gcc -static -march=native -O2 mainN23.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun223 $(pkg-config --cflags --libs wayland-client cairo cairo-ft freetype2) -lz -lm -lpthread -s
///////////////compilar en void//////////////////////////
$ZSYS/bin/musl-gcc -static mainN23.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun223 $(pkg-config --cflags --libs wayland-client cairo cairo-ft freetype2) -lz -lm -lpthread
/////////////////////////////
//bugs pendientes
Depurar los refrescos.
Recuperar las métricas que se hayan perdido.
Terminar los hovers.
Afinar los damage regions.
Volver a medir con perf cuando todo esté estable.
////primero compilar estatico cliente y server
musl-gcc -static mainN23.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o zmenun223 $(pkg-config --cflags --libs wayland-client cairo cairo-ft freetype2) -lm -lpthread
///cliente
$ZSYS/bin/musl-gcc -static cliente.c -o zmetrics-client -O3 -flto -ffunction-sections -fdata-sections -Wl,--gc-sections
///server
$ZSYS/bin/musl-gcc -static test.c metricas.c -o zmetrics-server -O3 -flto -ffunction-sections -fdata-sections -Wl,--gc-sections