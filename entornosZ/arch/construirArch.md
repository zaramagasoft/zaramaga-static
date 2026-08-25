unset CFLAGS
unset CXXFLAGS
unset CPPFLAGS
unset LDFLAGS
unset CPATH
unset C_INCLUDE_PATH
unset CPLUS_INCLUDE_PATH
unset LIBRARY_PATH
unset EXTRA_CFLAGS
unset LIBELF_FLAGS
unset CC
unset HOSTCC
unset PKG_CONFIG_PATH
unset CC CFLAGS CPPFLAGS C_INCLUDE_PATH CPLUS_INCLUDE_PATH CPATH LIBRARY_PATH PKG_CONFIG_PATH ZROOT ZSYS

1. Preparación del sistema y optimización global

Instalar herramientas oficiales de compilación y empaquetado:
Bash

sudo pacman -S --needed base-devel devtools git

Configurar optimizaciones de CPU para tu microarquitectura (/etc/makepkg.conf):
Bash

# Cambiar en /etc/makepkg.conf:
# CFLAGS="-march=native -O3 -pipe ..."
# MAKEFLAGS="-j$(nproc)"
# OPTIONS=(... lto)

2. Descargar y preparar la receta oficial

Crear directorio de trabajo y clonar la receta oficial con pkgctl:
Bash

mkdir -p ~/abs_build && cd ~/abs_build
pkgctl repo clone --protocol=https sway
cd sway
makepkg -si
makepkg -C -f -s --skippgpcheck
(Opcional) Importar la clave GPG del mantenedor si da fallo de firma:
Bash

gpg --recv-keys 0FDE7BE0E88F5E48

3. Limpieza de entorno y compilación

Asegurar un entorno limpio sin variables extrañas y compilar:
Bash

# Limpiar variables temporales si fuera necesario
unset CC CXX PATH PKG_CONFIG_PATH LD_LIBRARY_PATH
export PATH="/usr/local/sbin:/usr/local/bin:/usr/bin"

# Limpiar restos de intentos previos
makepkg -C

# Compilar e instalar dependencias
makepkg -si --skippgpcheck

4. Instalación del paquete binario y bloqueo

Instalar el paquete comprimido .pkg.tar.zst generado:
Bash

sudo pacman -U sway-*.pkg.tar.zst

Proteger el paquete contra sobrescrituras en /etc/pacman.conf:
Ini, TOML

# En /etc/pacman.conf:
IgnorePkg = sway

5. Actualización en el futuro

Cuando pacman -Syu te avise de que ignoró una versión nueva:
Bash

cd ~/abs_build/sway/sway
git pull
makepkg -si --skippgpcheck