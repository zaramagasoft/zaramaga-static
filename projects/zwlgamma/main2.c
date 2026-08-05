//
// Created by alb on 21/6/26.
//
#define _POSIX_C_SOURCE 200809L
#include "wlr-gamma-control-unstable-v1-client-protocol.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <math.h> // Necesario para pow()
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

// Handle global para la gestión de gamma
static struct zwlr_gamma_control_manager_v1 *gamma_control_manager = NULL;
struct wl_display *display = NULL;
static struct wl_list outputs;

struct output {
    struct wl_output *wl_output;
    struct zwlr_gamma_control_v1 *gamma_control;
    uint32_t ramp_size;
    int table_fd;
    uint16_t *table;
    struct wl_list link;
};

static void fill_gamma_table(uint16_t *table, uint32_t ramp_size, double contrast, double brightness, double gamma) {
    uint16_t *r = table;
    uint16_t *g = table + ramp_size;
    uint16_t *b = table + 2 * ramp_size;
    for (uint32_t i = 0; i < ramp_size; ++i) {
        double val = (double)i / (ramp_size - 1);
        val = contrast * pow(val, 1.0 / gamma) + (brightness - 1);
        if (val > 1.0) {
            val = 1.0;
        } else if (val < 0.0) {
            val = 0.0;
        }
        r[i] = g[i] = b[i] = (uint16_t)(UINT16_MAX * val);
    }
}

static int create_anonymous_file(off_t size) {
    char template[] = "/tmp/wlroots-shared-XXXXXX";
    int fd = mkstemp(template);
    if (fd < 0) {
        return -1;
    }

    int ret;
    do {
        errno = 0;
        ret = ftruncate(fd, size);
    } while (errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }

    unlink(template);
    return fd;
}

static int create_gamma_table(uint32_t ramp_size, uint16_t **table) {
    size_t table_size = ramp_size * 3 * sizeof(uint16_t);
    int fd = create_anonymous_file(table_size);
    if (fd < 0) {
        fprintf(stderr, "failed to create anonymous file\n");
        return -1;
    }

    void *data = mmap(NULL, table_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "failed to mmap()\n");
        close(fd);
        return -1;
    }

    *table = data;
    return fd;
}

static void gamma_control_handle_gamma_size(void *data, struct zwlr_gamma_control_v1 *gamma_control, uint32_t ramp_size) {
    struct output *output = data;
    output->ramp_size = ramp_size;
}

static void gamma_control_handle_failed(void *data, struct zwlr_gamma_control_v1 *gamma_control) {
    fprintf(stderr, "failed to set gamma table\n");
    exit(EXIT_FAILURE);
}

static const struct zwlr_gamma_control_v1_listener gamma_control_listener = {
    .gamma_size = gamma_control_handle_gamma_size,
    .failed = gamma_control_handle_failed,
};

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_output_interface.name) == 0) {
        struct output *output = calloc(1, sizeof(struct output));
        output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 1);
        wl_list_insert(&outputs, &output->link);
    } else if (strcmp(interface, zwlr_gamma_control_manager_v1_interface.name) == 0) {
        gamma_control_manager = wl_registry_bind(registry, name, &zwlr_gamma_control_manager_v1_interface, 1);
    }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    // Who cares?
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};

void wl_set_cbg(double contrast, double brightness, double gamma) {
    struct output *output;
    wl_list_for_each(output, &outputs, link) {
        output->table_fd = create_gamma_table(output->ramp_size, &output->table);
        if (output->table_fd < 0) {
            exit(EXIT_FAILURE);
        }

        fill_gamma_table(output->table, output->ramp_size, contrast, brightness, gamma);
        zwlr_gamma_control_v1_set_gamma(output->gamma_control, output->table_fd);
        close(output->table_fd);
    }

    wl_display_roundtrip(display);
}

static const char usage[] = "uso: motor_gamma [opciones]\n"
                            "  -h          muestra este mensaje de ayuda\n"
                            "  -c <valor>  ajusta el contraste (por defecto: 1.0)\n"
                            "  -b <valor>  ajusta el brillo (por defecto: 1.0)\n"
                            "  -g <valor>  ajusta el gamma (por defecto: 1.0)\n";

int main(int argc, char *argv[]) {
    double contrast = 1.0, brightness = 1.0, gamma = 1.0;
    int opt;

    // Procesar argumentos de la línea de comandos
    while ((opt = getopt(argc, argv, "hc:b:g:")) != -1) {
        switch (opt) {
        case 'c':
            contrast = strtod(optarg, NULL);
            break;
        case 'b':
            brightness = strtod(optarg, NULL);
            break;
        case 'g':
            gamma = strtod(optarg, NULL);
            break;
        case 'h':
        default:
            fprintf(stderr, "%s", usage);
            return opt == 'h' ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    wl_list_init(&outputs);

    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "cannot connect to display\n");
        exit(EXIT_FAILURE);
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (gamma_control_manager == NULL) {
        fprintf(stderr, "compositor doesn't support wlr-gamma-control-unstable-v1\n");
        return EXIT_FAILURE;
    }

    struct output *output;
    wl_list_for_each(output, &outputs, link) {
        output->gamma_control = zwlr_gamma_control_manager_v1_get_gamma_control(
                gamma_control_manager, output->wl_output);
        zwlr_gamma_control_v1_add_listener(
                output->gamma_control, &gamma_control_listener, output);
    }
    wl_display_roundtrip(display);

    // Aplicar los valores introducidos
    wl_set_cbg(contrast, brightness, gamma);

    // Bucle infinito necesario para mantener vivo el proceso
    // Si el proceso termina, Wayland destruirá el objeto gamma y restablecerá el brillo.
	wl_set_cbg(contrast, brightness, gamma);

	// --- NUEVO SISTEMA DINÁMICO (FIFO) ---
	const char *fifo_path = "/tmp/gamma_pipe";
	mkfifo(fifo_path, 0666); // Crea el archivo especial si no existe

	// O_RDWR evita que el pipe se cierre cuando tu dock no esté escribiendo
	int fifo_fd = open(fifo_path, O_RDWR | O_NONBLOCK);
	if (fifo_fd < 0) {
		fprintf(stderr, "Error abriendo el pipe FIFO\n");
		return EXIT_FAILURE;
	}

	struct pollfd fds[2];
	fds[0].fd = wl_display_get_fd(display); // Escucha a Wayland
	fds[0].events = POLLIN;
	fds[1].fd = fifo_fd;                    // Escucha a tu Dock
	fds[1].events = POLLIN;

	printf("Servicio Gamma iniciado. Escuchando en %s\n", fifo_path);

	while (1) {
		// Enviar peticiones pendientes a Wayland
		wl_display_dispatch_pending(display);
		wl_display_flush(display);

		// Esperar a que Wayland o tu Dock hablen
		if (poll(fds, 2, -1) < 0) continue;

		// Eventos de Wayland (el compositor dice algo)
		if (fds[0].revents & POLLIN) {
			if (wl_display_dispatch(display) == -1) break;
		}

		// Eventos del FIFO (tu dock ha enviado un nuevo valor)
		if (fds[1].revents & POLLIN) {
			char buf[64];
			ssize_t n = read(fifo_fd, buf, sizeof(buf) - 1);
			if (n > 0) {
				buf[n] = '\0';
				char cmd;
				double val;
				// Parsear comandos simples, ej: "b 0.5" o "c 1.2"
				if (sscanf(buf, "%c %lf", &cmd, &val) == 2) {
					if (cmd == 'b') brightness = val;
					if (cmd == 'c') contrast = val;
					if (cmd == 'g') gamma = val;

					// Aplicar dinámicamente
					wl_set_cbg(contrast, brightness, gamma);
				}
			}
		}
	}

	close(fifo_fd);
	unlink(fifo_path); // Limpiar al salir


    return EXIT_SUCCESS;
}