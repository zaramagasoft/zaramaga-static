/* ============================================================
 * HEADERS
 * ============================================================ */
#include <drm/drm.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <dirent.h>

/* DRM */


/* Input */
#include "zinput.h"

/* ============================================================
 * DEFINES
 * ============================================================ */
#define MAX_OUTPUTS     16
#define DRM_BUFFERS     2
#define CURSOR_SIZE     8

#ifndef DRM_CAP_ATOMIC
#define DRM_CAP_ATOMIC  0x3
#endif

/* ============================================================
 * ESTRUCTURAS
 * ============================================================ */

/* Buffer simple */
typedef struct {
    uint32_t fb_id;
    struct drm_mode_create_dumb create;
    struct drm_mode_map_dumb map;
    uint32_t *pixels;
} DrmBuffer;

/* Output con todo */
typedef struct {
    int fd;
    uint32_t connector_id;
    uint32_t crtc_id;
    drmModeModeInfo mode;
    drmModeCrtc *old_crtc;
    
    DrmBuffer buffers[DRM_BUFFERS];
    int front;
    int back;
    
    /* Atomic */
    int atomic_supported;
    uint32_t primary_plane;
    uint32_t cursor_plane;
} DrmOutput;

/* Sistema */
typedef struct {
    DrmOutput outputs[MAX_OUTPUTS];
    int count;
} DrmSystem;

/* ============================================================
 * GLOBALES
 * ============================================================ */
static int mouse_x = 0;
static int mouse_y = 0;
static volatile int running = 1;

/* ============================================================
 * PROTOTIPOS - MAÑANA LOS RELLENAS
 * ============================================================ */

/* DRM */
int drm_find_outputs(DrmSystem *system);
int drm_create_buffers(DrmOutput *output);
int drm_present(DrmOutput *output);
int drm_cleanup(DrmOutput *output);
int drm_init_atomic(DrmOutput *output);

/* Dibujo */
void draw_background(DrmOutput *output);
void draw_widget(DrmOutput *output);
void draw_cursor(DrmOutput *output);

/* Input */
int open_mouse(void);
void process_mouse(int fd);

/* ============================================================
 * FUNCIONES - MAÑANA LAS IMPLEMENTAS
 * ============================================================ */

/* ... (mañana escribes esto) ... */

/* ============================================================
 * MAIN - YA ESTÁ LISTO
 * ============================================================ */
void handle_sigint(int sig) {
    running = 0;
}

int main(void) {
    DrmSystem system = {0};
    DrmOutput *output;
    int mouse_fd = -1;
    struct pollfd fds[2];
    
    signal(SIGINT, handle_sigint);
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║     DRM/KMS Demo - Minimal                ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    /* 1. Buscar salidas */
    if (drm_find_outputs(&system) <= 0) {
        fprintf(stderr, "❌ No hay salidas DRM\n");
        return EXIT_FAILURE;
    }
    
    output = &system.outputs[0];
    
    /* 2. Crear buffers */
    if (drm_create_buffers(output) < 0) {
        drm_cleanup(output);
        return EXIT_FAILURE;
    }
    
    /* 3. Inicializar atomic (si se puede) */
    drm_init_atomic(output);
    
    /* 4. Abrir mouse */
    mouse_fd = open_mouse();
    if (mouse_fd < 0) {
        printf("⚠️  Mouse no encontrado\n");
    }
    
    /* 5. Posición inicial */
    mouse_x = output->buffers[0].create.width / 2;
    mouse_y = output->buffers[0].create.height / 2;
    
    /* 6. Dibujar y presentar */
    draw_background(output);
    draw_widget(output);
    draw_cursor(output);
    
    if (drm_present(output) < 0) {
        drm_cleanup(output);
        return EXIT_FAILURE;
    }
    
    printf("✅ Listo! Mueve el ratón | Ctrl+C para salir\n\n");
    
    /* 7. Bucle principal */
    fds[0].fd = output->fd;
    fds[0].events = POLLIN;
    if (mouse_fd >= 0) {
        fds[1].fd = mouse_fd;
        fds[1].events = POLLIN;
    }
    
    while (running) {
        int nfds = (mouse_fd >= 0) ? 2 : 1;
        int ret = poll(fds, nfds, 16);
        
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        if (mouse_fd >= 0 && (fds[1].revents & POLLIN)) {
            process_mouse(mouse_fd);
        }
        
        /* Redibujar si no hay plano cursor */
        if (!output->cursor_plane) {
            draw_background(output);
            draw_widget(output);
            draw_cursor(output);
            drm_present(output);
        }
    }
    
    /* 8. Limpiar */
    if (mouse_fd >= 0) close(mouse_fd);
    drm_cleanup(output);
    printf("\n👋 ¡Hasta luego!\n");
    
    return EXIT_SUCCESS;
}