#include <errno.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Globales para el control genérico de USB
int netlink_fd = -1;
char usb_action[32] = "";
char usb_dev_info[128] = "";

/* #include <stdio.h>
#include <string.h> */

void obtener_nombre_usb(const char *devpath, char *out_buf, size_t max_len) {
  char syspath[512];
  char manufacturer[64] = "";
  char product[64] = "";

  // 1. Leemos el fabricante (/sys/DEVPATH/manufacturer)
  snprintf(syspath, sizeof(syspath), "/sys%s/manufacturer", devpath);
  FILE *f = fopen(syspath, "r");
  if (f) {
    if (fgets(manufacturer, sizeof(manufacturer), f)) {
      manufacturer[strcspn(manufacturer, "\r\n")] = 0;
      printf(manufacturer); // Limpiar salto de línea
    }
    fclose(f);
  }

  // 2. Leemos el producto (/sys/DEVPATH/product)
  snprintf(syspath, sizeof(syspath), "/sys%s/product", devpath);
  f = fopen(syspath, "r");
  if (f) {
    if (fgets(product, sizeof(product), f)) {
      product[strcspn(product, "\r\n")] = 0; // Limpiar salto de línea
      printf(product);
    }
    fclose(f);
  }

  // 3. Formateamos el texto resultante
  if (strlen(manufacturer) > 0 || strlen(product) > 0) {
    snprintf(out_buf, max_len, "%s %s", manufacturer, product);
  } else {
    snprintf(out_buf, max_len, "Dispositivo USB");
  }
}
// Interfaz/función de prueba
void toastUSB() {
  char estado[160];
  snprintf(estado, sizeof(estado), "%s (%s)", usb_action, usb_dev_info);
  printf("\n[TOAST TRIGGERED] -> %s\n", estado);
}

int init_netlink_socket() {
  struct sockaddr_nl sa;
  memset(&sa, 0, sizeof(sa));
  sa.nl_family = AF_NETLINK;
  sa.nl_groups = 1; // Grupo 1 recibe los uevents del kernel

  // Lo abrimos en modo NO BLOQUEANTE y CLOSE-ON-EXEC
  int fd = socket(AF_NETLINK, SOCK_RAW | SOCK_NONBLOCK | SOCK_CLOEXEC,
                  NETLINK_KOBJECT_UEVENT);
  if (fd < 0)
    return -1;

  if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

int main() {
  netlink_fd = init_netlink_socket();
  if (netlink_fd < 0) {
    perror("Error al abrir socket Netlink");
    return 1;
  }

  printf("⚡ Escuchando eventos USB del kernel... Enchufa o quita un pendrive "
         "o dispositivo.\n");

  char buf[4096];

  while (1) {
    // Leemos el mensaje del kernel
    ssize_t len = recv(netlink_fd, buf, sizeof(buf) - 1, 0);

    if (len < 0) {
      // Como es NONBLOCK, si no hay datos dormimos 100ms para no saturar la CPU
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        usleep(100000);
        continue;
      }
      perror("Error en recv");
      break;
    }

    buf[len] = '\0'; // Aseguramos fin de cadena

    char *action = NULL;
    char *subsystem = NULL;
    char *devtype = NULL;

    printf("bufff %s\n", buf);

    // El primer string trae "add@/devices/..." o "remove@/devices/..."
    if (strncmp(buf, "add@", 4) == 0) {
      action = "CONECTADO";
    } else if (strncmp(buf, "remove@", 7) == 0) {
      action = "DESCONECTADO";
    }

    // El payload son variables separadas por '\0'
    char *ptr = buf;
    while (ptr < buf + len) {
      if (strncmp(ptr, "SUBSYSTEM=", 10) == 0)
        subsystem = ptr + 10;
      if (strncmp(ptr, "DEVTYPE=", 8) == 0)
        devtype = ptr + 8;
      ptr += strlen(ptr) + 1;
    }
    // FILTRO: Solo disparamos el cartel para el dispositivo USB principal
    if (action && subsystem && strcmp(subsystem, "usb") == 0 && devtype &&
        strcmp(devtype, "usb_device") == 0) {
      char *devpath = strchr(buf, '/');

      if (devpath) {
        if (strcmp(action, "CONECTADO") == 0) {
          // Leemos la marca/modelo directamente de /sys
          obtener_nombre_usb(devpath, usb_dev_info, sizeof(usb_dev_info));
        } else {
          // Al desconectar, la carpeta en /sys se borra al instante,
          // así que ponemos un texto genérico de desconexión.
          snprintf(usb_dev_info, sizeof(usb_dev_info), "Dispositivo extraído");
        }
        snprintf(usb_action, sizeof(usb_action), "%s", action);
        snprintf(usb_dev_info, sizeof(usb_dev_info), "Dispositivo USB");

        // Llamamos a la función que dispara la notificación
        toastUSB();
      }
    }
  }

  close(netlink_fd);
  return 0;
}