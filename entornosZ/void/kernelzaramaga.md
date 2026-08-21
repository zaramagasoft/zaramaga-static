kernel
///ZRAM HABILITAR //////
#!/usr/bin/env bash

sudo sh -c '
echo lz4 > /sys/block/zram0/comp_algorithm
echo $((24 * 1024 * 1024 * 1024)) > /sys/block/zram0/disksize
mkswap /dev/zram0
swapon -p 100 /dev/zram0
'

echo
echo '========== ZRAM =========='
cat /sys/block/zram0/comp_algorithm
cat /sys/block/zram0/disksize

echo
echo '========== SWAP =========='
swapon --show
free -h
////ejemplo de src
sudo xbps-install -f -R /home/alb/void-packages/hostdir/binpkgs gcc
///steam/////gamescope
gamescope -W 1920 -H 1080 -w 1920 -h 1080 -e -r 60 -f --mangoapp -- steam -steamos3 -gamepadui

////ahora conlimpieza drivers///////////
y con localmodconfig

real    4m37.286s
user    75m46.063s
sys     12m13.463s
[alb@void-linux-sata linux-6.18.44]$ 

////////////////////////////////////////
recompilar con src openssl-devel , elfutils-devel
real    12m11.215s
user    235m0.761s
sys     37m32.356s
[alb@void-linux-sata linux-6.18.44]$ 
cd ~/void-packages
./xbps-src pkg ncurses-devel
xi ncurses-devel
xbps-query -H
time ./xbps-src pkg openssl-devel elfutils-devel
xi openssl-devel elfutils-devel
sudo xbps-pkgdb -m hold openssl-devel
sudo xbps-pkgdb -m hold elfutils-devel
 

///////////ejecutar/////////// y medir
time make -j$(nproc) \
    KCFLAGS="-O2 -pipe -march=native"
    
    
real    12m7.937s
user    234m44.389s
sys     37m25.062s
[alb@void-linux-sata linux-6.18.44]$ 

gcc --version | head -1
-rw-r--r-- 1 alb alb 15M Aug 15 11:46 arch/x86/boot/bzImage
gcc (GCC) 14.2.1 20250405
[alb@void-linux-sata linux-6.18.44]$ nproc
24
[alb@void-linux-sata linux-6.18.44]$ 
