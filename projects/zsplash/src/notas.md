/////////lanzar/////////////
sudo openvt -f -c 2 -s -w -- ./p4 
git config --global user.name "alb1122"
git config --global user.email "alb1122oli@gmail.com"

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
