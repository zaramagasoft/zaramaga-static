#define _GNU_SOURCE
#include "xdg-shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <cairo.h>
#include <wayland-client.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

// ============================================================================
// LÍNEA 1: WAYLAND + XDG SHELL
// ============================================================================
static struct wl_display    *g_wl_display    = NULL;
static struct wl_registry   *g_wl_registry   = NULL;
static struct wl_compositor *g_wl_compositor = NULL;
static struct wl_surface    *g_wl_surface    = NULL;
static struct wl_shm        *g_wl_shm        = NULL;
static struct xdg_wm_base   *g_xdg_wm_base   = NULL;
static struct xdg_surface   *g_xdg_surface   = NULL;
static struct xdg_toplevel  *g_xdg_toplevel  = NULL;

// ============================================================================
// LÍNEA 2: SHM MEMORY POOL
// ============================================================================
static struct wl_shm_pool *g_shm_pool   = NULL;
static struct wl_buffer   *g_shm_buffer = NULL;
static uint32_t           *g_pixel_data = NULL;
static int                 g_shm_fd     = -1;
static size_t              g_shm_size   = 0;
static int                 g_width      = 800;
static int                 g_height     = 600;

// ============================================================================
// LÍNEA 3: CAIRO & NUKLEAR CONTEXTS
// ============================================================================
static cairo_surface_t *g_cairo_surface = NULL;
static cairo_t         *g_cairo_ctx     = NULL;

static struct nk_context g_nk_ctx;
static struct nk_font_atlas g_nk_atlas;
static struct nk_user_font g_nk_user_font;


// --- LISTENERS WAYLAND ---
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = { .ping = xdg_wm_base_ping };

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    xdg_surface_ack_configure(xdg_surface, serial);
    wl_surface_attach(g_wl_surface, g_shm_buffer, 0, 0);
    wl_surface_damage(g_wl_surface, 0, 0, g_width, g_height);
    wl_surface_commit(g_wl_surface);
}
static const struct xdg_surface_listener xdg_surface_listener = { .configure = xdg_surface_configure };

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                   int32_t width, int32_t height, struct wl_array *states) {
    if (width > 0 && height > 0) {
        g_width = width;
        g_height = height;
    }
}
static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) { exit(0); }
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        g_wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        g_wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        g_xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(g_xdg_wm_base, &xdg_wm_base_listener, NULL);
    }
}
static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

// --- RENDERIZADOR DE COMANDOS NUKLEAR CON CAIRO ---
// --- RENDERIZADOR DE COMANDOS NUKLEAR CON CAIRO ---
static void nk_cairo_render(struct nk_context *ctx, cairo_t *cr) {
  const struct nk_command *cmd;
  nk_foreach(cmd, ctx) {
    switch (cmd->type) {
    case NK_COMMAND_NOP:
      break;

    case NK_COMMAND_RECT: {
      const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
      cairo_set_source_rgba(cr, r->color.r / 255.0, r->color.g / 255.0,
                            r->color.b / 255.0, r->color.a / 255.0);
      cairo_set_line_width(cr, r->line_thickness);
      cairo_rectangle(cr, r->x, r->y, r->w, r->h);
      cairo_stroke(cr);
    } break;

    case NK_COMMAND_RECT_FILLED: {
      const struct nk_command_rect_filled *r =
          (const struct nk_command_rect_filled *)cmd;
      cairo_set_source_rgba(cr, r->color.r / 255.0, r->color.g / 255.0,
                            r->color.b / 255.0, r->color.a / 255.0);
      cairo_rectangle(cr, r->x, r->y, r->w, r->h);
      cairo_fill(cr);
    } break;

    case NK_COMMAND_CIRCLE_FILLED: {
      const struct nk_command_circle_filled *c =
          (const struct nk_command_circle_filled *)cmd;
      cairo_set_source_rgba(cr, c->color.r / 255.0, c->color.g / 255.0,
                            c->color.b / 255.0, c->color.a / 255.0);
      cairo_arc(cr, c->x + c->w / 2.0, c->y + c->h / 2.0, c->w / 2.0, 0,
                2 * M_PI);
      cairo_fill(cr);
    } break;

    case NK_COMMAND_TEXT: {
      const struct nk_command_text *t = (const struct nk_command_text *)cmd;
      cairo_set_source_rgba(cr, t->foreground.r / 255.0,
                            t->foreground.g / 255.0, t->foreground.b / 255.0,
                            t->foreground.a / 255.0);
      cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                             CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, t->height);
      cairo_move_to(cr, t->x, t->y + t->height - 2);
      cairo_show_text(cr, (const char *)t->string);
    } break;

    default:
      break;
    }
  }
  nk_clear(ctx);
}

// --- RENDER DE ESCENA (CUBO + NUKLEAR) ---
void render_frame(void)
{
    cairo_set_source_rgb(g_cairo_ctx, 0.1, 0.1, 0.12);
    cairo_paint(g_cairo_ctx);

    nk_input_begin(&g_nk_ctx);
    nk_input_end(&g_nk_ctx);

    if (nk_begin(&g_nk_ctx,
                 "ZGUI Control Panel",
                 nk_rect(20, 20, 240, 220),
                 NK_WINDOW_BORDER |
                 NK_WINDOW_MOVABLE |
                 NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(&g_nk_ctx, 25, 1);
        nk_label(&g_nk_ctx,
                 "Controles de ZaramagaOS",
                 NK_TEXT_LEFT);
    }

    nk_end(&g_nk_ctx);

    nk_cairo_render(&g_nk_ctx, g_cairo_ctx);

    wl_surface_attach(g_wl_surface, g_shm_buffer, 0, 0);
    wl_surface_damage(g_wl_surface, 0, 0, g_width, g_height);
    wl_surface_commit(g_wl_surface);
}

// --- INICIALIZACIÓN ---
int init_wayland_line1(void) {
    g_wl_display = wl_display_connect(NULL);
    if (!g_wl_display) return -1;

    g_wl_registry = wl_display_get_registry(g_wl_display);
    wl_registry_add_listener(g_wl_registry, &registry_listener, NULL);
    wl_display_roundtrip(g_wl_display);

    g_wl_surface = wl_compositor_create_surface(g_wl_compositor);
    g_xdg_surface = xdg_wm_base_get_xdg_surface(g_xdg_wm_base, g_wl_surface);
    xdg_surface_add_listener(g_xdg_surface, &xdg_surface_listener, NULL);

    g_xdg_toplevel = xdg_surface_get_toplevel(g_xdg_surface);
    xdg_toplevel_add_listener(g_xdg_toplevel, &xdg_toplevel_listener, NULL);
    xdg_toplevel_set_title(g_xdg_toplevel, "ZGUI - Cubo 3D + Nuklear UI");

    wl_surface_commit(g_wl_surface);
    wl_display_roundtrip(g_wl_display);
    return 0;
}

static int create_anonymous_file(size_t size) {
    int fd = memfd_create("zgui-shm", MFD_CLOEXEC);
    if (fd < 0) return -1;
    if (ftruncate(fd, size) < 0) { close(fd); return -1; }
    return fd;
}

int init_shm_line2(int width, int height) {
    g_width = width; g_height = height;
    int stride = width * 4;
    g_shm_size = stride * height;

    g_shm_fd = create_anonymous_file(g_shm_size);
    if (g_shm_fd < 0) return -1;

    g_pixel_data = mmap(NULL, g_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
    if (g_pixel_data == MAP_FAILED) return -1;

    g_shm_pool = wl_shm_create_pool(g_wl_shm, g_shm_fd, g_shm_size);
    g_shm_buffer = wl_shm_pool_create_buffer(g_shm_pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    return 0;
}

int init_cairo_and_nuklear_line3(void) {
  int stride = g_width * 4;
  g_cairo_surface = cairo_image_surface_create_for_data(
      (unsigned char *)g_pixel_data, CAIRO_FORMAT_ARGB32, g_width, g_height,
      stride);
  g_cairo_ctx = cairo_create(g_cairo_surface);

  // 1. Inicializar Atlas de fuentes de Nuklear
  nk_font_atlas_init_default(&g_nk_atlas);
  nk_font_atlas_begin(&g_nk_atlas);

  // 2. Crear la fuente bitmap por defecto
  struct nk_font *nk_font = nk_font_atlas_add_default(&g_nk_atlas, 13, NULL);

  // 3. Finalizar atlas (genera las métricas de glifos)
  const void *image;
  int img_w, img_h;
  image = nk_font_atlas_bake(&g_nk_atlas, &img_w, &img_h, NK_FONT_ATLAS_ALPHA8);
  nk_font_atlas_end(&g_nk_atlas, nk_handle_ptr(NULL), NULL);

  // 4. Copiar las métricas a la estructura global persistente
  if (nk_font) {
    g_nk_user_font = nk_font->handle;
  }

  // 5. Inicializar contexto vinculando la fuente fija
  if (!nk_init_default(&g_nk_ctx, &g_nk_user_font)) {
    fprintf(stderr, "Error al inicializar Nuklear.\n");
    return -1;
  }

  printf("¡Línea 3 (Cairo + Nuklear) OK! Fuente en memoria fija.\n");
  return 0;
}

int main(int argc, char const *argv[]) {
    printf("Iniciando ZGUI Pipeline con Nuklear...\n");
    if (init_wayland_line1() < 0) return 1;
    if (init_shm_line2(800, 600) < 0) return 1;
    if (init_cairo_and_nuklear_line3() < 0) return 1;

    
    while (wl_display_dispatch_pending(g_wl_display) != -1) {
        render_frame();
       
        wl_display_flush(g_wl_display);
        usleep(16000); // ~60 FPS
    }

    nk_font_atlas_clear(&g_nk_atlas);
    nk_free(&g_nk_ctx);
    cairo_destroy(g_cairo_ctx);
    cairo_surface_destroy(g_cairo_surface);
    return 0;
}
