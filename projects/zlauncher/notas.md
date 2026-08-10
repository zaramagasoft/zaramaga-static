//////////////////////
sudo openvt -f -c 2 -s -w -- ./zl2
 
//// compilar ////
musl-gcc -static -O2 \
    z2.c \
    -I$ZSYS/include \
    -L$ZSYS/lib \
    -ldrm \
    -o zl2

musl-gcc -static -O2     z.c      -I$ZSYS/include     -I$ZSYS/include/freetype2     -L$ZSYS/lib     -lcairo -lfreetype -ldrm -lpixman-1 -lpng18 -lz -lm     -o zl

$ZSYS/bin/musl-gcc zsplash2.c -static -I$ZSYS/include -I$ZSYS/include/cairo -L$ZSYS/lib -lcairo -l:libpng18.a -lfreetype -lpixman-1 -ldrm -lz -lm -lpthread -o p4 -Os -flto -ffunction-sections -fdata-sections -Wl,--gc-sections

$ZSYS/bin/musl-gcc pruebas.c -static -I$ZSYS/include -
I$ZSYS/include/cairo -L$ZSYS/lib -lcairo -l:libpng18.a -lfreetype -lpixman-1 -ldrm
 -lz -lm -lpthread -o pruebas
[alb@zaramagaOS zlauncher]$ ./pruebas 
SDK Zaramaga Static OK
drmAvailable() = 0
Pixman OK
FreeType OK
libpng 1.8.0.git
Cairo 1.18.4
Cairo OK
[alb@zaramagaOS zlauncher]$ 

$ZSYS/bin/musl-gcc     -static     pruebas.c     -I$ZSYS/include     -L$ZSYS/lib     -ldrm     -lpixman-1     -lfreetype     -lpng18     -lz     -lm     -o pruebas

$ZSYS/bin/musl-gcc -static \
    pruebas.c \
    -I$ZSYS/include \
    -L$ZSYS/lib \
    -ldrm \
    -lpixman-1 \
    -lfreetype \
    -lm \
    -o pruebas