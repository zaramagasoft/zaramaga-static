#define _GNU_SOURCE
#include "xdg-shell.h"
#include <cairo/cairo.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

// ============================================================================
// LÍNEA 1: WAYLAND + XDG SHELL (64 BYTES ALINEADOS)
// ============================================================================

static struct wl_display *g_wl_display = NULL;
static struct wl_registry *g_wl_registry = NULL;
static struct wl_compositor *g_wl_compositor = NULL;
static struct wl_surface *g_wl_surface = NULL;
static struct wl_shm *g_wl_shm = NULL;
static struct xdg_wm_base *g_xdg_wm_base = NULL;
static struct xdg_surface *g_xdg_surface = NULL;
static struct xdg_toplevel *g_xdg_toplevel = NULL;

typedef struct __attribute__((aligned(64))) {
  struct wl_display **const display;
  struct wl_registry **const registry;
  struct wl_compositor **const compositor;
  struct wl_surface **const surface;
  struct wl_shm **const shm;
  struct xdg_wm_base **const xdg_wm_base;
  struct xdg_surface **const xdg_surface;
  struct xdg_toplevel **const xdg_toplevel;
} WaylandPointers;

const WaylandPointers wayland_ptrs = {.display = &g_wl_display,
                                      .registry = &g_wl_registry,
                                      .compositor = &g_wl_compositor,
                                      .surface = &g_wl_surface,
                                      .shm = &g_wl_shm,
                                      .xdg_wm_base = &g_xdg_wm_base,
                                      .xdg_surface = &g_xdg_surface,
                                      .xdg_toplevel = &g_xdg_toplevel};

// ============================================================================
// LÍNEA 2: SHM MEMORY POOL & PIXELS (64 BYTES ALINEADOS)
// ============================================================================

static struct wl_shm_pool *g_shm_pool = NULL;
static struct wl_buffer *g_shm_buffer = NULL;
static uint32_t *g_pixel_data = NULL;
static int g_shm_fd = -1;
static size_t g_shm_size = 0;
static int g_width = 800;
static int g_height = 600;

typedef struct __attribute__((aligned(64))) {
  struct wl_shm_pool **const pool;
  struct wl_buffer **const buffer;
  uint32_t **const pixels;
  int *const fd;
  size_t *const size;
} SHMPointers;

const SHMPointers shm_ptrs = {.pool = &g_shm_pool,
                              .buffer = &g_shm_buffer,
                              .pixels = &g_pixel_data,
                              .fd = &g_shm_fd,
                              .size = &g_shm_size};

// --- LISTENERS DE WAYLAND ---

// ============================================================================
// LÍNEA 3: CAIRO SURFACE DIRECTO A L1/L2 (64 BYTES ALINEADOS)
// ============================================================================
static cairo_surface_t *g_cairo_surface = NULL;
static cairo_t *g_cairo_ctx = NULL;

// Vértices del Cubo 3D (-1 a 1)
typedef struct {
  float x, y, z;
} Vec3;
typedef struct {
  float x, y;
} Vec2;
// --- LISTENERS DE XDG SURFACE / TOPLEVEL ---

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
                                  uint32_t serial) {
  xdg_surface_ack_configure(xdg_surface, serial);

  // Al recibir la confirmación del compositor, publicamos el primer frame
  wl_surface_attach(g_wl_surface, g_shm_buffer, 0, 0);
  wl_surface_damage(g_wl_surface, 0, 0, g_width, g_height);
  wl_surface_commit(g_wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data,
                                   struct xdg_toplevel *xdg_toplevel,
                                   int32_t width, int32_t height,
                                   struct wl_array *states) {
  if (width > 0 && height > 0) {
    g_width = width;
    g_height = height;
  }
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
  exit(0);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};

static Vec3 cube_nodes[8] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                             {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};

static int cube_edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

// --- FUNCIÓN DE DIBUJO CON CAIRO ---
void render_cube(float angle) {
  // 1. Limpiar el lienzo (Fondo oscuro)
  cairo_set_source_rgb(g_cairo_ctx, 0.1, 0.1, 0.12);
  cairo_paint(g_cairo_ctx);

  // 2. Proyección 3D a 2D
  Vec2 projected[8];
  float rad = angle * (M_PI / 180.0f);
  float cos_a = cosf(rad), sin_a = sinf(rad);

  for (int i = 0; i < 8; i++) {
    // Rotación en Y y X
    float x = cube_nodes[i].x * cos_a - cube_nodes[i].z * sin_a;
    float z = cube_nodes[i].x * sin_a + cube_nodes[i].z * cos_a;
    float y = cube_nodes[i].y * cos_a - z * sin_a;
    z = cube_nodes[i].y * sin_a + z * cos_a;

    // Perspectiva simple
    float distance = 3.5f;
    float fov = 400.0f;
    projected[i].x = (x * fov) / (z + distance) + (g_width / 2.0f);
    projected[i].y = (y * fov) / (z + distance) + (g_height / 2.0f);
  }

  // 3. Dibujar aristas con Cairo
  cairo_set_line_width(g_cairo_ctx, 3.0);
  cairo_set_source_rgb(g_cairo_ctx, 0.2, 0.8, 1.0); // Azul neón

  for (int i = 0; i < 12; i++) {
    Vec2 p1 = projected[cube_edges[i][0]];
    Vec2 p2 = projected[cube_edges[i][1]];
    cairo_move_to(g_cairo_ctx, p1.x, p1.y);
    cairo_line_to(g_cairo_ctx, p2.x, p2.y);
  }
  cairo_stroke(g_cairo_ctx);

  // Asignar el buffer a la superficie de Wayland
  wl_surface_attach(g_wl_surface, g_shm_buffer, 0, 0);
  wl_surface_damage(g_wl_surface, 0, 0, g_width, g_height);
  wl_surface_commit(g_wl_surface);
}

// --- INICIALIZAR CAIRO SOBRE EL SHM MAPEADO ---
int init_cairo_line3(void) {
  int stride = g_width * 4;

  // Vinculación directa: Cairo escribe sobre la misma memoria que lee Wayland
  g_cairo_surface = cairo_image_surface_create_for_data(
      (unsigned char *)g_pixel_data, CAIRO_FORMAT_ARGB32, g_width, g_height,
      stride);

  if (cairo_surface_status(g_cairo_surface) != CAIRO_STATUS_SUCCESS) {
    fprintf(stderr, "Error al crear la superficie de Cairo.\n");
    return -1;
  }

  g_cairo_ctx = cairo_create(g_cairo_surface);
  printf("¡Línea 3 (Cairo en L1/L2) OK! Contexto listo.\n");
  return 0;
}

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *wm_base,
                             uint32_t serial) {
  xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t name, const char *interface,
                            uint32_t version) {
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    g_wl_compositor =
        wl_registry_bind(registry, name, &wl_compositor_interface, 4);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    g_wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    g_xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(g_xdg_wm_base, &xdg_wm_base_listener, NULL);
  }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                   uint32_t name) {}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

// --- INICIALIZADORES DE LÍNEAS DE CACHÉ ---

int init_wayland_line1(void) {
  g_wl_display = wl_display_connect(NULL);
  if (!g_wl_display) {
    fprintf(stderr, "Error: No se pudo conectar a Wayland.\n");
    return -1;
  }

  g_wl_registry = wl_display_get_registry(g_wl_display);
  wl_registry_add_listener(g_wl_registry, &registry_listener, NULL);
  wl_display_roundtrip(g_wl_display);

  if (!g_wl_compositor || !g_wl_shm || !g_xdg_wm_base) {
    fprintf(stderr, "Error: Faltan interfaces esenciales de Wayland.\n");
    return -1;
  }

  g_wl_surface = wl_compositor_create_surface(g_wl_compositor);
  g_xdg_surface = xdg_wm_base_get_xdg_surface(g_xdg_wm_base, g_wl_surface);
  g_xdg_toplevel = xdg_surface_get_toplevel(g_xdg_surface);

  g_wl_surface = wl_compositor_create_surface(g_wl_compositor);
  g_xdg_surface = xdg_wm_base_get_xdg_surface(g_xdg_wm_base, g_wl_surface);

  // Registrar listeners de la superficie
  xdg_surface_add_listener(g_xdg_surface, &xdg_surface_listener, NULL);

  g_xdg_toplevel = xdg_surface_get_toplevel(g_xdg_surface);

  // Registrar listeners del toplevel y poner título
  xdg_toplevel_add_listener(g_xdg_toplevel, &xdg_toplevel_listener, NULL);
  xdg_toplevel_set_title(g_xdg_toplevel, "ZGUI - Cubo 3D Cairo");

  // Commit inicial para desencadenar el evento 'configure' del compositor
  wl_surface_commit(g_wl_surface);
  wl_display_roundtrip(g_wl_display);

  //wl_surface_commit(g_wl_surface);
  //wl_display_roundtrip(g_wl_display);

  printf("¡Línea 1 (Wayland) OK! Tabla en: %p\n", (void *)&wayland_ptrs);
  return 0;
}

static int create_anonymous_file(size_t size) {
  int fd = memfd_create("zgui-shm", MFD_CLOEXEC);
  if (fd < 0)
    return -1;
  if (ftruncate(fd, size) < 0) {
    close(fd);
    return -1;
  }
  return fd;
}

int init_shm_line2(int width, int height) {
  g_width = width;
  g_height = height;

  int stride = width * 4;
  g_shm_size = stride * height;

  g_shm_fd = create_anonymous_file(g_shm_size);
  if (g_shm_fd < 0)
    return -1;

  g_pixel_data =
      mmap(NULL, g_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
  if (g_pixel_data == MAP_FAILED) {
    close(g_shm_fd);
    return -1;
  }

  struct wl_shm *shm_iface = *wayland_ptrs.shm;
  g_shm_pool = wl_shm_create_pool(shm_iface, g_shm_fd, g_shm_size);
  g_shm_buffer = wl_shm_pool_create_buffer(g_shm_pool, 0, width, height, stride,
                                           WL_SHM_FORMAT_ARGB8888);

  printf("¡Línea 2 (SHM) OK! Buffer mmap en: %p | Tabla en: %p\n",
         (void *)g_pixel_data, (void *)&shm_ptrs);
  return 0;
}

int main(int argc, char const *argv[]) {
  printf("Iniciando ZGUI Pipeline...\n");
  if (init_wayland_line1() < 0)
    return 1;
  if (init_shm_line2(800, 600) < 0)
    return 1;
  if (init_cairo_line3() < 0)
    return 1;

  printf("Líneas L1, L2 y L3 operativas. Renderizando marco...\n");

  float angle = 0.0f;
  while (wl_display_dispatch_pending(g_wl_display) != -1) {
    render_cube(angle);
    angle += 1.5f;
    if (angle >= 360.0f)
      angle = 0.0f;

    wl_display_flush(g_wl_display);
    usleep(16000); // ~60 FPS
  }

  // Limpieza
  cairo_destroy(g_cairo_ctx);
  cairo_surface_destroy(g_cairo_surface);
  return 0;
}