sudo openvt -f -c 2 -s -w -- ./r.sh
 
///gamescope///
gamescope -- bash -c "sleep 5  && GSK_RENDERER=cairo WAYLAND_DISPLAY=wayland-1 ./zxdg"
musl-gcc -static -O2     zgui.c xdg-shell.c     -I"$ZSYS/include"     -I"$ZSYS/include/cairo"     -L"$ZSYS/lib"     -Wl,--start-group     -lcairo     -lpixman-1     -lfreetype     -lpng     -lwayland-client     -lffi     -lm     -ldl     -lrt -lz    -lpthread     -Wl,--end-group     -o zxdg

gcc -O3 -march=native -Wall zgui2.c xdg-shell.c -o albway2     $(pkg-config --cflags --libs wayland-client cairo) -lm


gcc mainN.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o ztoast     $(pkg-config --cflags --libs wayland-client wayland-cursor cairo)     -lm -O0
gcc mainN.c wlr-layer-shell-unstable-v1.c xdg-shell.c -o ztoast     $(pkg-config --cflags --libs wayland-client wayland-cursor cairo)     -lm -O3
