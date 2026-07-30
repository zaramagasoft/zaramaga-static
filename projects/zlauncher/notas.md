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