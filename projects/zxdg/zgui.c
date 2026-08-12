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
#include <linux/input-event-codes.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define MAX_OUTPUTS 16
#include "nuklear.h"
#include "zlayout.h"
//include "zoutput.h"

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
static struct wl_seat *g_wl_seat = NULL;
static struct wl_pointer *g_wl_pointer = NULL;


typedef struct
{
    struct wl_output *output;

    uint32_t name;
    uint32_t version;

    int32_t x;
    int32_t y;

    int32_t physical_width;
    int32_t physical_height;

    int32_t mode_width;
    int32_t mode_height;

    int32_t refresh;

    int32_t scale;
    int32_t transform;

    char *output_name;
    char *description;

    int mode_valid;
    int geometry_valid;
    int done;

} ZOutput;

static ZOutput g_outputs[MAX_OUTPUTS];
static int g_output_count = 0;

static double g_mouse_x = 0.0;
static double g_mouse_y = 0.0;

static const struct wl_pointer_listener pointer_listener;
// ============================================================================
// LÍNEA 2: SHM MEMORY POOL
// ============================================================================
static struct wl_shm_pool *g_shm_pool = NULL;
static struct wl_buffer *g_shm_buffer = NULL;
static uint32_t *g_pixel_data = NULL;
static int g_shm_fd = -1;
static size_t g_shm_size = 0;
static int g_width = 800;
static int g_height = 600;
static int g_configured = 0;
static int g_resize_pending = 0;
// ============================================================================
// LÍNEA 3: CAIRO & NUKLEAR CONTEXTS
// ============================================================================
static cairo_surface_t *g_cairo_surface = NULL;
static cairo_t *g_cairo_ctx = NULL;

static struct nk_context g_nk_ctx;
static struct nk_font_atlas g_nk_atlas;
static struct nk_user_font g_nk_user_font;

static int g_mouse_left = 0;
static int g_mouse_right = 0;
static int g_mouse_middle = 0;

////////////// PROTOTYPES///////////////

///////////////////////////////////////

//////////////////////////////////////

static int create_anonymous_file(size_t size);

////////////////////////////////////////
//////////////////////////////////////
static void zoutput_dump(void)
{
    printf("\n");
    printf("========================================\n");
    printf("ZOUTPUT: %d monitor(es)\n", g_output_count);
    printf("========================================\n");

    for (int i = 0; i < g_output_count; i++)
    {
        ZOutput *z = &g_outputs[i];

        printf("\nOUTPUT %d\n", i);

        printf(
            "  Name:        %s\n",
            z->output_name ? z->output_name : "(sin nombre)");

        printf(
            "  Description: %s\n",
            z->description ? z->description : "(sin descripcion)");

        printf(
            "  Position:    %d, %d\n",
            z->x,
            z->y);

        printf(
            "  Resolution:  %d x %d\n",
            z->mode_width,
            z->mode_height);

        printf(
            "  Refresh:     %.3f Hz\n",
            z->refresh / 1000.0);

        printf(
            "  Scale:       %d\n",
            z->scale);

        printf(
            "  Transform:   %d\n",
            z->transform);

        printf(
            "  Physical:    %d x %d mm\n",
            z->physical_width,
            z->physical_height);
    }

    printf("\n");
}
static void zoutput_geometry(
    void *data,
    struct wl_output *output,
    int32_t x,
    int32_t y,
    int32_t physical_width,
    int32_t physical_height,
    int32_t subpixel,
    const char *make,
    const char *model,
    int32_t transform)
{
    ZOutput *z = data;

    z->x = x;
    z->y = y;

    z->physical_width = physical_width;
    z->physical_height = physical_height;

    z->transform = transform;

    z->geometry_valid = 1;
}
static void zoutput_mode(
    void *data,
    struct wl_output *output,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t refresh)
{
    ZOutput *z = data;

    /*
     * Nos interesa el modo actual.
     */

    if (flags & WL_OUTPUT_MODE_CURRENT)
    {
        z->mode_width = width;
        z->mode_height = height;
        z->refresh = refresh;
        z->mode_valid = 1;
    }
}
static void zoutput_scale(
    void *data,
    struct wl_output *output,
    int32_t factor)
{
    ZOutput *z = data;

    z->scale = factor;
}
static void zoutput_name(
    void *data,
    struct wl_output *output,
    const char *name)
{
    ZOutput *z = data;

    free(z->output_name);

    z->output_name = strdup(name);
}
static void zoutput_description(
    void *data,
    struct wl_output *output,
    const char *description)
{
    ZOutput *z = data;

    free(z->description);

    z->description = strdup(description);
}
static void zoutput_done(
    void *data,
    struct wl_output *output)
{
    ZOutput *z = data;

    z->done = 1;
}
static const struct wl_output_listener zoutput_listener =
    {
        .geometry = zoutput_geometry,
        .mode = zoutput_mode,
        .done = zoutput_done,
        .scale = zoutput_scale,
        .name = zoutput_name,
        .description = zoutput_description};
static int resize_shm(int width, int height)
{
    int stride = width * 4;
    size_t size = (size_t)stride * height;

    printf("SHM RESIZE: %dx%d\n",
           width,
           height);

    /*
     * Destruir buffer anterior
     */

    if (g_shm_buffer)
    {
        wl_buffer_destroy(g_shm_buffer);
        g_shm_buffer = NULL;
    }

    if (g_shm_pool)
    {
        wl_shm_pool_destroy(g_shm_pool);
        g_shm_pool = NULL;
    }

    if (g_pixel_data)
    {
        munmap(
            g_pixel_data,
            g_shm_size);

        g_pixel_data = NULL;
    }

    if (g_shm_fd >= 0)
    {
        close(g_shm_fd);
        g_shm_fd = -1;
    }

    /*
     * Crear nuevo SHM
     */

    g_shm_fd = create_anonymous_file(size);

    if (g_shm_fd < 0)
    {
        perror("create_anonymous_file");
        return -1;
    }

    g_pixel_data = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        g_shm_fd,
        0);

    if (g_pixel_data == MAP_FAILED)
    {
        perror("mmap");

        g_pixel_data = NULL;

        close(g_shm_fd);
        g_shm_fd = -1;

        return -1;
    }

    g_shm_size = size;

    /*
     * Crear pool
     */

    g_shm_pool = wl_shm_create_pool(
        g_wl_shm,
        g_shm_fd,
        size);

    if (!g_shm_pool)
    {
        fprintf(stderr, "wl_shm_create_pool fallo\n");
        return -1;
    }

    /*
     * Crear buffer
     */

    g_shm_buffer =
        wl_shm_pool_create_buffer(
            g_shm_pool,
            0,
            width,
            height,
            stride,
            WL_SHM_FORMAT_ARGB8888);

    if (!g_shm_buffer)
    {
        fprintf(stderr,
                "wl_shm_pool_create_buffer fallo\n");

        return -1;
    }

    /*
     * Recrear Cairo
     */

    if (g_cairo_ctx)
    {
        cairo_destroy(g_cairo_ctx);
        g_cairo_ctx = NULL;
    }

    if (g_cairo_surface)
    {
        cairo_surface_destroy(g_cairo_surface);
        g_cairo_surface = NULL;
    }

    g_cairo_surface =
        cairo_image_surface_create_for_data(
            (unsigned char *)g_pixel_data,
            CAIRO_FORMAT_ARGB32,
            width,
            height,
            stride);

    if (cairo_surface_status(g_cairo_surface) != CAIRO_STATUS_SUCCESS)
    {
        fprintf(stderr,
                "Error creando Cairo surface\n");

        return -1;
    }

    g_cairo_ctx =
        cairo_create(g_cairo_surface);

    if (cairo_status(g_cairo_ctx) != CAIRO_STATUS_SUCCESS)
    {
        fprintf(stderr,
                "Error creando Cairo context\n");

        return -1;
    }

    printf("SHM RESIZE OK\n");

    return 0;
}
// --- LISTENERS WAYLAND ---
static void seat_capabilities(
    void *data,
    struct wl_seat *seat,
    uint32_t capabilities)
{
    printf("SEAT CAPABILITIES: 0x%x\n", capabilities);
    fflush(stdout);

    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        printf("SEAT: POINTER DISPONIBLE\n");
        fflush(stdout);

        if (!g_wl_pointer)
        {
            g_wl_pointer = wl_seat_get_pointer(seat);

            wl_pointer_add_listener(
                g_wl_pointer,
                &pointer_listener,
                NULL);

            printf("POINTER LISTENER INSTALADO\n");
            fflush(stdout);
        }
    }
}
static void seat_name(
    void *data,
    struct wl_seat *seat,
    const char *name)
{
    printf("SEAT NAME: %s\n", name);
    fflush(stdout);
}
static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = seat_name,
};
static void pointer_button(void *data,
                           struct wl_pointer *pointer,
                           uint32_t serial,
                           uint32_t time,
                           uint32_t button,
                           uint32_t state)
{
    if (button == BTN_LEFT)
    {
        g_mouse_left = (state == WL_POINTER_BUTTON_STATE_PRESSED);

        printf("MOUSE LEFT: %s\n",
               g_mouse_left ? "DOWN" : "UP");
        fflush(stdout);
    }
}
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
    xdg_surface_ack_configure(
        xdg_surface,
        serial);

    g_configured = 1;
}
static const struct xdg_surface_listener xdg_surface_listener = {.configure = xdg_surface_configure};

static void xdg_toplevel_configure(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array *states)
{
    if (width > 0 && height > 0)
    {
        if (width != g_width || height != g_height)
        {
            printf("XDG RESIZE: %dx%d -> %dx%d\n",
                   g_width,
                   g_height,
                   width,
                   height);

            g_width = width;
            g_height = height;

            g_resize_pending = 1;
        }
    }
}
static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) { exit(0); }
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};
static void pointer_enter(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    struct wl_surface *surface,
    wl_fixed_t sx,
    wl_fixed_t sy)
{
    g_mouse_x = wl_fixed_to_double(sx);
    g_mouse_y = wl_fixed_to_double(sy);

    printf("MOUSE ENTER %.1f %.1f\n",
           g_mouse_x,
           g_mouse_y);
}

static void pointer_leave(
    void *data,
    struct wl_pointer *pointer,
    uint32_t serial,
    struct wl_surface *surface)
{
    printf("MOUSE LEAVE\n");
}

static void pointer_motion(void *data,
                           struct wl_pointer *pointer,
                           uint32_t time,
                           wl_fixed_t sx,
                           wl_fixed_t sy)
{
    g_mouse_x = wl_fixed_to_double(sx);
    g_mouse_y = wl_fixed_to_double(sy);

    printf("MOUSE: %.1f %.1f\n", g_mouse_x, g_mouse_y);
    fflush(stdout);
}

static void pointer_axis(
    void *data,
    struct wl_pointer *pointer,
    uint32_t time,
    uint32_t axis,
    wl_fixed_t value)
{
    printf("MOUSE AXIS=%u VALUE=%.1f\n",
           axis,
           wl_fixed_to_double(value));
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
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
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        uint32_t seat_version = version < 3 ? version : 3;

        g_wl_seat = wl_registry_bind(
            registry,
            name,
            &wl_seat_interface,
            seat_version);

        wl_seat_add_listener(
            g_wl_seat,
            &seat_listener,
            NULL);

        printf("WAYLAND: SEAT OK version=%u\n", seat_version);
        fflush(stdout);
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        if (g_output_count >= MAX_OUTPUTS)
            return;

        ZOutput *z = &g_outputs[g_output_count];

        memset(z, 0, sizeof(*z));

        z->name = name;
        z->version = version;

        z->scale = 1;

        uint32_t output_version = version < 4 ? version : 4;

        z->output =
            wl_registry_bind(
                registry,
                name,
                &wl_output_interface,
                output_version);

        wl_output_add_listener(
            z->output,
            &zoutput_listener,
            z);

        printf(
            "WAYLAND: OUTPUT encontrado name=%u version=%u\n",
            name,
            output_version);

        g_output_count++;
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
    if (!g_configured)
        return;

    if (g_resize_pending)
    {
        if (resize_shm(g_width, g_height) < 0)
        {
            fprintf(stderr,
                    "Error redimensionando SHM\n");

            return;
        }

        g_resize_pending = 0;
    }
    // 1. Limpieza de superficie con Cairo
    cairo_set_source_rgb(g_cairo_ctx, 0.1, 0.1, 0.12);
    cairo_paint(g_cairo_ctx);
    static const char *anchor_names[] =
        {
            "TOP LEFT",
            "TOP",
            "TOP RIGHT",

            "LEFT",
            "CENTER",
            "RIGHT",

            "BOTTOM LEFT",
            "BOTTOM",
            "BOTTOM RIGHT"};
    for (int i = 0; i < 9; i++)
    {
        ZWidget widget =
            {
                .anchor = (ZAnchor)i,
                .x = 20,
                .y = 20,
                .width = 140,
                .height = 50};

        ZRect rect =
            zlayout_resolve(
                &widget,
                g_width,
                g_height);

        if (nk_begin(
                &g_nk_ctx,
                anchor_names[i],
                nk_rect(
                    rect.x,
                    rect.y,
                    rect.width,
                    rect.height),
                NK_WINDOW_BORDER |
                    NK_WINDOW_TITLE))
        {
            nk_layout_row_dynamic(
                &g_nk_ctx,
                20,
                1);

            nk_label(
                &g_nk_ctx,
                anchor_names[i],
                NK_TEXT_CENTERED);
          
        }

        nk_end(&g_nk_ctx);
    }
    // // 2. Iniciar el bloque de entradas en Nuklear
    // nk_input_begin(&g_nk_ctx);

    // // Posición del puntero
    // nk_input_motion(&g_nk_ctx, (int)g_mouse_x, (int)g_mouse_y);

    // // Botones del ratón
    // nk_input_button(&g_nk_ctx, NK_BUTTON_LEFT, (int)g_mouse_x, (int)g_mouse_y, g_mouse_left);
    // nk_input_button(&g_nk_ctx, NK_BUTTON_RIGHT, (int)g_mouse_x, (int)g_mouse_y, g_mouse_right);
    // nk_input_button(&g_nk_ctx, NK_BUTTON_MIDDLE, (int)g_mouse_x, (int)g_mouse_y, g_mouse_middle);

    // nk_input_end(&g_nk_ctx);

    // // 3. Definición de la Interfaz Nuklear
    // if (nk_begin(&g_nk_ctx,
    //              "ZGUI Control Panel",
    //              nk_rect(20, 20, 240, 220),
    //              NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE))
    // {
    //     nk_layout_row_dynamic(&g_nk_ctx, 25, 1);
    //     nk_label(&g_nk_ctx, "Controles de ZaramagaOS", NK_TEXT_LEFT);

    //     nk_layout_row_dynamic(&g_nk_ctx, 25, 1);
    //     if (nk_button_label(&g_nk_ctx, "Botón 1"))
    //     {
    //         printf("¡Botón 1 pulsado!\n");
    //         fflush(stdout);
    //     }

    //     nk_layout_row_dynamic(&g_nk_ctx, 25, 1);
    //     if (nk_button_label(&g_nk_ctx, "Botón 2"))
    //     {
    //         printf("¡Botón 2 pulsado!\n");
    //         fflush(stdout);
    //     }
    // }
    // nk_end(&g_nk_ctx);

    // // 4. Dibujar Nuklear mediante Cairo
    nk_cairo_render(&g_nk_ctx, g_cairo_ctx);

    // 5. Enviar frame a Wayland
    wl_surface_attach(g_wl_surface, g_shm_buffer, 0, 0);
    wl_surface_damage(g_wl_surface, 0, 0, g_width, g_height);
    wl_surface_commit(g_wl_surface);
}

// --- INICIALIZACIÓN ---
int init_wayland_line1(void)
{
    g_wl_display = wl_display_connect(NULL);
    if (!g_wl_display)
        return -1;
    
    g_wl_registry = wl_display_get_registry(g_wl_display);
    wl_registry_add_listener(g_wl_registry, &registry_listener, NULL);
    wl_display_roundtrip(g_wl_display);
    // zoutput_print(&g_zoutputs);

    g_wl_surface = wl_compositor_create_surface(g_wl_compositor);
    g_xdg_surface = xdg_wm_base_get_xdg_surface(g_xdg_wm_base, g_wl_surface);
    xdg_surface_add_listener(g_xdg_surface, &xdg_surface_listener, NULL);

    g_xdg_toplevel = xdg_surface_get_toplevel(g_xdg_surface);
    xdg_toplevel_add_listener(g_xdg_toplevel, &xdg_toplevel_listener, NULL);
    xdg_toplevel_set_title(g_xdg_toplevel, "ZGUI - Cubo 3D + Nuklear UI");

    wl_surface_commit(g_wl_surface);
    wl_display_roundtrip(g_wl_display);
    if (!g_wl_seat)
    {
        fprintf(stderr, "WAYLAND: NO SEAT\n");
        return -1;
    }

    zoutput_dump();

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

int init_shm_line2(int width, int height)
{
    g_width = width;
    g_height = height;
    int stride = width * 4;
    g_shm_size = stride * height;

    g_shm_fd = create_anonymous_file(g_shm_size);
    if (g_shm_fd < 0)
        return -1;

    g_pixel_data = mmap(NULL, g_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
    if (g_pixel_data == MAP_FAILED)
        return -1;

    g_shm_pool = wl_shm_create_pool(g_wl_shm, g_shm_fd, g_shm_size);
    g_shm_buffer = wl_shm_pool_create_buffer(g_shm_pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    g_shm_buffer = wl_shm_pool_create_buffer(
        g_shm_pool,
        0,
        width,
        height,
        stride,
        WL_SHM_FORMAT_ARGB8888);

    if (!g_shm_buffer)
        return -1;

    g_resize_pending = 0;

    return 0;
}

int init_cairo_and_nuklear_line3(void)
{
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
    if (nk_font)
    {
        g_nk_user_font = nk_font->handle;
    }

    // 5. Inicializar contexto vinculando la fuente fija
    if (!nk_init_default(&g_nk_ctx, &g_nk_user_font))
    {
        fprintf(stderr, "Error al inicializar Nuklear.\n");
        return -1;
    }

    printf("¡Línea 3 (Cairo + Nuklear) OK! Fuente en memoria fija.\n");
    return 0;
}

int main(int argc, char const *argv[])
{
    printf("Iniciando ZGUI Pipeline con Nuklear...\n");
    if (init_wayland_line1() < 0)
        return 1;
    if (init_shm_line2(800, 600) < 0)
        return 1;
    if (init_cairo_and_nuklear_line3() < 0)
        return 1;

    // === CLAVE 1: Forzar la negociación inicial con XDG-Shell ===
    wl_display_roundtrip(g_wl_display);

    // Dibuja el primer frame explícitamente tras la negociación
    render_frame();

    // === CLAVE 2: Bucle de eventos funcional ===
    while (wl_display_dispatch(g_wl_display) != -1)
    {
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
