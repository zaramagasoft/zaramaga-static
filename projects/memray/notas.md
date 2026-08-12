/////////////////////////////////////////
esto es raylib a la memoria platform memory
musl-gcc -static -O0     waymix.c  xdg-shell.c ./libraylib.a     -I"$ZSYS/include"     -I"$ZSYS/include/cairo"     -L"$ZSYS/lib"     -Wl,--start-group     -lcairo     -lpixman-1     -lfreetype     -lpng     -lwayland-client     -lffi     -lm     -ldl     -lrt -lz    -lpthread     -Wl,--end-group     -o waymix2


////////////////////////////////////////////
/////////////////////////////////ojo//////////////
$ZSYS/bin/musl-gcc -static test.c ./libraylib.a -Iinclude -lm -o test-static

//////////way es un entorno wayland minimisimo.......
 gcc way.c xdg-shell.c \
    -I. \
    -lwayland-client \
    -o way
/////////////compilar way.c que es una ventana wayland a secas//////////////
gcc way.c -lwayland-client -lrt -o way