#!/usr/bin/env bash

# 1. Comprobar si el módulo zram está cargado en el kernel
if ! lsmod | grep -q "^zram"; then
    echo "Cargando el módulo del kernel zram..."
    sudo modprobe zram
fi

# 2. Comprobar si /dev/zram0 ya está montado como SWAP
if swapon --show | grep -q "/dev/zram0"; then
    echo "[!] /dev/zram0 ya está configurado y activo como SWAP."
else
    echo "[+] Configurando y activando /dev/zram0..."

    # Desactivar por seguridad si existía pero no estaba en SWAP
    sudo swapoff /dev/zram0 2>/dev/null

    sudo sh -c '
    # Desactivar temporalmente para poder cambiar el algoritmo si estuviera libre
    echo lz4 > /sys/block/zram0/comp_algorithm 2>/dev/null || true
    echo $((24 * 1024 * 1024 * 1024)) > /sys/block/zram0/disksize
    mkswap /dev/zram0 >/dev/null
    swapon -p 100 /dev/zram0
    '
    echo "[+] ZRAM activado con éxito."
fi

# 3. Mostrar el estado final del sistema
echo
echo '========== ZRAM =========='
echo -n "Algoritmo: " && cat /sys/block/zram0/comp_algorithm
echo -n "Tamaño:    " && cat /sys/block/zram0/disksize

echo
echo '========== SWAP =========='
swapon --show

echo
echo '========== MEMORIA =========='
free -h
