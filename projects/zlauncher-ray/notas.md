sudo openvt -f -c 2 -s -w -- ./zlauncher-ray

musl-gcc -static -O2     main.c     -I"$ZSYS/include"     -L"$ZSYS/lib"     -Wl,--start-group     -lraylib     -ldrm     -lpthread     -lrt     -lm     -ldl     -Wl,--end-group     -o zlauncher-ray