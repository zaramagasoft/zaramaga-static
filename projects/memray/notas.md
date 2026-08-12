/////////////////////////////////////////
esto es raylib a la memoria platform memory

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