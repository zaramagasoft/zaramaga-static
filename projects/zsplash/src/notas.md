$ZSYS/bin/musl-gcc zsplash.c -static -I$ZSYS/include -I$ZSYS/include/cairo -L$ZSYS/lib -lcairo -l:libpng18.a -lfreetype -lpixman-1 -ldrm -lz -lm -lpthread -o pruebas3 -Os -flto -ffunction-sections -fdata-sections -Wl,--gc-sections

ojo y despues strip casi 3 megas.


$ZSYS/bin/musl-gcc zsplash.c -static -I$ZSYS/include -I$ZSYS/include/cairo -L$ZSYS/lib -lcairo -l:libpng18.a -lfreetype -lpixman-1 -ldrm -lz -lm -lpthread -o pruebas3

$ZSYS/bin/musl-gcc zsplash.c \
-static \
-I$ZSYS/include \
-I$ZSYS/include/cairo \
-L$ZSYS/lib \
-lcairo \
-l:libpng18.a \
-lfreetype \
-lpixman-1 \
-ldrm \
-lz \
-lm \
-lpthread \
-o pruebas3

 $ZSYS/bin/musl-gcc     -static     zsplash.c     -I$ZSYS/include     -L$ZSYS/lib     -ldrm         -lm     -o pruebas2 -s -O2
