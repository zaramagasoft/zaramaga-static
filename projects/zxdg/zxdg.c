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
static struct wl_display *g_wl_display = NULL;
static struct wl_registry *g_wl_registry = NULL;
static struct wl_compositor *g_wl_compositor = NULL;
static struct wl_surface *g_wl_surface = NULL;
static struct wl_shm *g_wl_shm = NULL;
static struct xdg_wm_base *g_xdg_wm_base = NULL;
static struct xdg_surface *g_xdg_surface = NULL;
static struct xdg_toplevel *g_xdg_toplevel = NULL;

// ============================================================================
// LÍNEA 2: SHM MEMORY POOL
// ============================================================================
#define BUFFER_COUNT 2

typedef struct
{
    struct wl_buffer *wl_buffer;
    uint32_t *pixels;

    cairo_surface_t *cairo_surface;
    cairo_t *cairo_ctx;

    int busy;
} ShmBuffer;

static struct wl_shm_pool *g_shm_pool = NULL;
static int g_shm_fd = -1;
static size_t g_shm_size = 0;
static int g_width = 800;
static int g_height = 600;
static ShmBuffer g_buffers[BUFFER_COUNT];

static int g_configured = 0;
static int g_frame_ready = 1;
static int g_running = 1;

// ============================================================================
// LÍNEA 3: CAIRO & NUKLEAR CONTEXTS
// ============================================================================

static struct nk_context g_nk_ctx;
static struct nk_font_atlas g_nk_atlas;
static struct nk_user_font g_nk_user_font;

/// prototipos ////////
//////////////////////

static ShmBuffer *get_free_buffer(void);

static void frame_done(void *data,
                       struct wl_callback *callback,
                       uint32_t callback_data)
{
    g_frame_ready = 1;
    wl_callback_destroy(callback);
}
static int init_cairo_buffers(void)
{
    int stride = g_width * 4;

    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        g_buffers[i].cairo_surface =
            cairo_image_surface_create_for_data(
                (unsigned char *)g_buffers[i].pixels,
                CAIRO_FORMAT_ARGB32,
                g_width,
                g_height,
                stride);

        if (cairo_surface_status(g_buffers[i].cairo_surface) != CAIRO_STATUS_SUCCESS)
        {
            return -1;
        }

        g_buffers[i].cairo_ctx =
            cairo_create(g_buffers[i].cairo_surface);

        if (cairo_status(g_buffers[i].cairo_ctx) != CAIRO_STATUS_SUCCESS)
        {
            return -1;
        }
    }

    return 0;
}
static const struct wl_callback_listener frame_listener = {
    .done = frame_done};
static void buffer_release(void *data, struct wl_buffer *buffer)
{
    ShmBuffer *buf = data;

    buf->busy = 0;
}

static const struct wl_buffer_listener buffer_listener = {
    .release = buffer_release};
// --- LISTENERS WAYLAND ---
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial)
{
    xdg_wm_base_pong(wm_base, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = {.ping = xdg_wm_base_ping};

static void xdg_surface_configure(
    void *data,
    struct xdg_surface *xdg_surface,
    uint32_t serial)
{
    printf("XDG: CONFIGURE serial=%u\n", serial);

    xdg_surface_ack_configure(
        xdg_surface,
        serial);

    g_configured = 1;

    printf("XDG: configured=1\n");
}
static const struct xdg_surface_listener xdg_surface_listener = {.configure = xdg_surface_configure};

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
                                   int32_t width, int32_t height, struct wl_array *states)
{
    if (width > 0 && height > 0)
    {
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
                            uint32_t name, const char *interface, uint32_t version)
{
    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        g_wl_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        g_wl_shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
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
static void nk_cairo_render(struct nk_context *ctx, cairo_t *cr)
{
    const struct nk_command *cmd;
    nk_foreach(cmd, ctx)
    {
        switch (cmd->type)
        {
        case NK_COMMAND_NOP:
            break;

        case NK_COMMAND_RECT:
        {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            cairo_set_source_rgba(cr, r->color.r / 255.0, r->color.g / 255.0,
                                  r->color.b / 255.0, r->color.a / 255.0);
            cairo_set_line_width(cr, r->line_thickness);
            cairo_rectangle(cr, r->x, r->y, r->w, r->h);
            cairo_stroke(cr);
        }
        break;

        case NK_COMMAND_RECT_FILLED:
        {
            const struct nk_command_rect_filled *r =
                (const struct nk_command_rect_filled *)cmd;
            cairo_set_source_rgba(cr, r->color.r / 255.0, r->color.g / 255.0,
                                  r->color.b / 255.0, r->color.a / 255.0);
            cairo_rectangle(cr, r->x, r->y, r->w, r->h);
            cairo_fill(cr);
        }
        break;

        case NK_COMMAND_CIRCLE_FILLED:
        {
            const struct nk_command_circle_filled *c =
                (const struct nk_command_circle_filled *)cmd;
            cairo_set_source_rgba(cr, c->color.r / 255.0, c->color.g / 255.0,
                                  c->color.b / 255.0, c->color.a / 255.0);
            cairo_arc(cr, c->x + c->w / 2.0, c->y + c->h / 2.0, c->w / 2.0, 0,
                      2 * M_PI);
            cairo_fill(cr);
        }
        break;

        case NK_COMMAND_TEXT:
        {
            const struct nk_command_text *t = (const struct nk_command_text *)cmd;
            cairo_set_source_rgba(cr, t->foreground.r / 255.0,
                                  t->foreground.g / 255.0, t->foreground.b / 255.0,
                                  t->foreground.a / 255.0);
            cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                                   CAIRO_FONT_WEIGHT_NORMAL);
            cairo_set_font_size(cr, t->height);
            cairo_move_to(cr, t->x, t->y + t->height - 2);
            cairo_show_text(cr, (const char *)t->string);
        }
        break;

        default:
            break;
        }
    }
    nk_clear(ctx);
}

// --- RENDER DE ESCENA (CUBO + NUKLEAR) ---
void render_frame(void)
{
    ShmBuffer *buf = get_free_buffer();

    if (!buf)
        return;

    buf->busy = 1;

    cairo_t *cr = buf->cairo_ctx;

    /* PRUEBA BRUTA: TODO ROJO */
    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    cairo_paint(cr);

    cairo_surface_flush(buf->cairo_surface);

    struct wl_callback *callback =
        wl_surface_frame(g_wl_surface);

    wl_callback_add_listener(
        callback,
        &frame_listener,
        NULL);

    g_frame_ready = 0;

    wl_surface_attach(
        g_wl_surface,
        buf->wl_buffer,
        0,
        0);

    wl_surface_damage(
        g_wl_surface,
        0,
        0,
        g_width,
        g_height);

    wl_surface_commit(g_wl_surface);
}

// --- INICIALIZACIÓN ---
int init_wayland_line1(void)
{
    printf("WAYLAND: connect\n");

    // g_wl_display = wl_display_connect(NULL);
    // if (!g_wl_display)
    //     return -1;

    // printf("WAYLAND: display OK\n");
    g_wl_display = wl_display_connect(NULL);

    if (!g_wl_display)
    {
        fprintf(stderr, "WAYLAND: wl_display_connect() FALLÓ\n");
        perror("wl_display_connect");
        return -1;
    }
    g_wl_registry = wl_display_get_registry(g_wl_display);

    wl_registry_add_listener(
        g_wl_registry,
        &registry_listener,
        NULL);

    printf("WAYLAND: roundtrip registry\n");

    if (wl_display_roundtrip(g_wl_display) < 0)
        return -1;

    printf("WAYLAND: compositor=%p shm=%p xdg=%p\n",
           (void *)g_wl_compositor,
           (void *)g_wl_shm,
           (void *)g_xdg_wm_base);

    g_wl_surface =
        wl_compositor_create_surface(g_wl_compositor);

    if (!g_wl_surface)
        return -1;

    printf("WAYLAND: surface=%p\n",
           (void *)g_wl_surface);

    g_xdg_surface =
        xdg_wm_base_get_xdg_surface(
            g_xdg_wm_base,
            g_wl_surface);

    if (!g_xdg_surface)
        return -1;

    xdg_surface_add_listener(
        g_xdg_surface,
        &xdg_surface_listener,
        NULL);

    g_xdg_toplevel =
        xdg_surface_get_toplevel(
            g_xdg_surface);

    if (!g_xdg_toplevel)
        return -1;

    xdg_toplevel_add_listener(
        g_xdg_toplevel,
        &xdg_toplevel_listener,
        NULL);

    xdg_toplevel_set_title(
        g_xdg_toplevel,
        "ZaramagaOS ZLauncher");

    printf("WAYLAND: initial commit\n");

    /*
     * Este commit provoca el configure inicial.
     */
    wl_surface_commit(g_wl_surface);

    printf("WAYLAND: waiting initial configure\n");

    if (wl_display_roundtrip(g_wl_display) < 0)
        return -1;

    printf("WAYLAND: roundtrip finished configured=%d\n",
           g_configured);

    return 0;
}

static int create_anonymous_file(size_t size)
{
    int fd = memfd_create("zgui-shm", MFD_CLOEXEC);
    if (fd < 0)
        return -1;
    if (ftruncate(fd, size) < 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}
static ShmBuffer *get_free_buffer(void)
{
    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        if (!g_buffers[i].busy)
            return &g_buffers[i];
    }

    return NULL;
}

int init_shm_line2(int width, int height)
{
    printf("SHM: inicio %dx%d\n", width, height);

    g_width = width;
    g_height = height;

    int stride = width * 4;

    g_shm_size =
        (size_t)stride * height * BUFFER_COUNT;

    printf("SHM: size=%zu stride=%d\n",
           g_shm_size, stride);

    g_shm_fd =
        create_anonymous_file(g_shm_size);

    if (g_shm_fd < 0)
    {
        perror("SHM: memfd");
        return -1;
    }

    printf("SHM: fd=%d\n", g_shm_fd);

    g_shm_pool =
        wl_shm_create_pool(
            g_wl_shm,
            g_shm_fd,
            g_shm_size);

    if (!g_shm_pool)
    {
        fprintf(stderr,
                "SHM: wl_shm_create_pool FALLÓ\n");
        return -1;
    }

    printf("SHM: pool OK\n");

    /*
     * Mapeamos TODO el área SHM una sola vez.
     */
    void *shm_data =
        mmap(
            NULL,
            g_shm_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            g_shm_fd,
            0);

    if (shm_data == MAP_FAILED)
    {
        perror("SHM: mmap");
        return -1;
    }

    printf("SHM: mmap global OK -> %p\n",
           shm_data);

    size_t buffer_size =
        (size_t)stride * height;

    /*
     * Creamos los dos buffers dentro
     * del mismo bloque de memoria.
     */
    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        size_t offset =
            (size_t)i * buffer_size;

        g_buffers[i].pixels =
            (uint32_t *)((unsigned char *)shm_data + offset);

        printf(
            "SHM: buffer %d pixels=%p offset=%zu\n",
            i,
            (void *)g_buffers[i].pixels,
            offset);

        g_buffers[i].wl_buffer =
            wl_shm_pool_create_buffer(
                g_shm_pool,
                offset,
                width,
                height,
                stride,
                WL_SHM_FORMAT_ARGB8888);

        if (!g_buffers[i].wl_buffer)
        {
            fprintf(
                stderr,
                "SHM: create_buffer %d FALLÓ\n",
                i);

            return -1;
        }

        printf("SHM: wl_buffer %d OK\n", i);

        g_buffers[i].busy = 0;

        wl_buffer_add_listener(
            g_buffers[i].wl_buffer,
            &buffer_listener,
            &g_buffers[i]);
    }

    printf("SHM: TODO OK\n");

    return 0;
}

int init_cairo_and_nuklear_line3(void)
{
    if (init_cairo_buffers() < 0)
        return -1;

    nk_font_atlas_init_default(&g_nk_atlas);
    nk_font_atlas_begin(&g_nk_atlas);

    struct nk_font *font =
        nk_font_atlas_add_default(&g_nk_atlas, 13, NULL);

    int img_w, img_h;

    nk_font_atlas_bake(
        &g_nk_atlas,
        &img_w,
        &img_h,
        NK_FONT_ATLAS_ALPHA8);

    nk_font_atlas_end(
        &g_nk_atlas,
        nk_handle_ptr(NULL),
        NULL);

    if (font)
        g_nk_user_font = font->handle;

    if (!nk_init_default(&g_nk_ctx, &g_nk_user_font))
    {
        fprintf(stderr, "Error inicializando Nuklear\n");
        return -1;
    }

    printf("Cairo + Nuklear OK\n");

    return 0;
}

int main(int argc, char const *argv[])
{
    printf("Iniciando ZGUI Pipeline con Nuklear...\n");

    printf("MAIN 1\n");

    if (init_wayland_line1() < 0)
        return 1;

    printf("MAIN 2 - WAYLAND OK\n");

    if (init_shm_line2(800, 600) < 0)
        return 1;

    printf("MAIN 3 - SHM OK\n");

    if (init_cairo_and_nuklear_line3() < 0)
        return 1;

    printf("MAIN 4 - CAIRO + NUKLEAR OK\n");

    printf("MAIN 5 - RENDER\n");

    render_frame();

    printf("MAIN 6 - RENDER OK\n");

    while (g_running)
    {
        if (wl_display_dispatch(g_wl_display) < 0)
            break;

        if (g_frame_ready)
        {
            render_frame();
            wl_display_flush(g_wl_display);
        }
    }

    return 0;
}
