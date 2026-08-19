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