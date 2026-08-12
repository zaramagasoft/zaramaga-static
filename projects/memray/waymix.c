#include "raylib.h"
#include "rlgl.h"
#include "xdg-shell.h"
#include <wayland-client.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 800
#define HEIGHT 600
#define SIZE (WIDTH * HEIGHT * 4)

// Framebuffer permanente en BSS
static unsigned char g_framebuffer[WIDTH * HEIGHT * 4];

// Puntero constante al framebuffer
unsigned char *const g_pBuffer = g_framebuffer;

static unsigned char framebuffer[SIZE];

static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct wl_surface *surface;
static struct wl_shm_pool *pool;
static struct wl_buffer *buffer;

static struct xdg_wm_base *xdg_wm_base;
static struct xdg_surface *xdg_surface;
static struct xdg_toplevel *xdg_toplevel;
static int configured = 0;

static void xdg_ping(
    void *data,
    struct xdg_wm_base *xdg_wm_base,
    uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static void xdg_surface_configure(
    void *data,
    struct xdg_surface *xdg_surface,
    uint32_t serial)
{
    xdg_surface_ack_configure(
        xdg_surface,
        serial);

    configured = 1;
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_ping};

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure};
static void registry_global(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0)
    {
        compositor = wl_registry_bind(
            registry,
            name,
            &wl_compositor_interface,
            4);
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        shm = wl_registry_bind(
            registry,
            name,
            &wl_shm_interface,
            1);
    }
    else if (strcmp(interface, "xdg_wm_base") == 0)
    {
        xdg_wm_base = wl_registry_bind(
            registry,
            name,
            &xdg_wm_base_interface,
            1);
    }
}

static void registry_remove(
    void *data,
    struct wl_registry *registry,
    uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    registry_remove};

int main(void)
{
    printf("Wayland 800x600\n");

    /* -----------------------------------------
     * Conectar con Wayland
     * ----------------------------------------- */

    display = wl_display_connect(NULL);

    if (!display)
    {
        fprintf(stderr, "No se pudo conectar a Wayland\n");
        return 1;
    }

    /* -----------------------------------------
     * Obtener globals
     * ----------------------------------------- */

    struct wl_registry *registry =
        wl_display_get_registry(display);

    wl_registry_add_listener(
        registry,
        &registry_listener,
        NULL);

    wl_display_roundtrip(display);

    xdg_wm_base_add_listener(
        xdg_wm_base,
        &xdg_wm_base_listener,
        NULL);

    if (!compositor || !shm || !xdg_wm_base)
    {
        fprintf(stderr,
                "Faltan compositor, SHM o xdg_wm_base\n");
        return 1;
    }

    /* -----------------------------------------
     * Crear wl_surface
     * ----------------------------------------- */

    surface = wl_compositor_create_surface(compositor);

    if (!surface)
    {
        fprintf(stderr, "No se pudo crear wl_surface\n");
        return 1;
    }

    /* -----------------------------------------
     * Crear xdg_surface
     * ----------------------------------------- */

    xdg_surface =
        xdg_wm_base_get_xdg_surface(
            xdg_wm_base,
            surface);

    if (!xdg_surface)
    {
        fprintf(stderr, "No se pudo crear xdg_surface\n");
        return 1;
    }

    /* Listener del configure */
    xdg_surface_add_listener(
        xdg_surface,
        &xdg_surface_listener,
        NULL);

    /* -----------------------------------------
     * Crear ventana toplevel
     * ----------------------------------------- */

    xdg_toplevel =
        xdg_surface_get_toplevel(
            xdg_surface);

    if (!xdg_toplevel)
    {
        fprintf(stderr, "No se pudo crear xdg_toplevel\n");
        return 1;
    }

    xdg_toplevel_set_title(
        xdg_toplevel,
        "memray - 800x600");

    /* -----------------------------------------
     * PRIMER COMMIT
     *
     * MUY IMPORTANTE:
     * todavía SIN BUFFER.
     *
     * Esto hace que el compositor mande
     * el primer configure.
     * ----------------------------------------- */

    wl_surface_commit(surface);

    /* Esperar configure inicial */

    wl_display_roundtrip(display);

    if (!configured)
    {
        fprintf(stderr,
                "No recibimos el configure inicial\n");
        return 1;
    }

    printf("Configure recibido\n");

    /* -----------------------------------------
     * Crear SHM
     * ----------------------------------------- */

    int fd = shm_open(
        "/ztest-wayland",
        O_CREAT | O_RDWR,
        0600);

    if (fd < 0)
    {
        perror("shm_open");
        return 1;
    }

    shm_unlink("/ztest-wayland");

    if (ftruncate(fd, SIZE) < 0)
    {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    /* -----------------------------------------
     * Mapear memoria
     * ----------------------------------------- */

    unsigned char *pixels = mmap(
        NULL,
        SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0);

    if (pixels == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    /* -----------------------------------------
     * Fondo negro
     * ----------------------------------------- */

    memset(
        pixels,
        0,
        SIZE);

    /* -----------------------------------------
     * Rectángulo verde
     *
     * XRGB8888
     * ----------------------------------------- */

    for (int y = 200; y < 400; y++)
    {
        for (int x = 250; x < 550; x++)
        {
            unsigned char *p =
                pixels +
                (y * WIDTH + x) * 4;

            p[0] = 0;   /* B */
            p[1] = 255; /* G */
            p[2] = 0;   /* R */
            p[3] = 255; /* X */
        }
    }

    /* -----------------------------------------
     * Crear pool Wayland
     * sobre nuestra memoria SHM
     * ----------------------------------------- */

    pool = wl_shm_create_pool(
        shm,
        fd,
        SIZE);

    if (!pool)
    {
        fprintf(stderr,
                "No se pudo crear wl_shm_pool\n");
        return 1;
    }

    /* -----------------------------------------
     * Crear wl_buffer
     * ----------------------------------------- */

    buffer =
        wl_shm_pool_create_buffer(
            pool,
            0,
            WIDTH,
            HEIGHT,
            WIDTH * 4,
            WL_SHM_FORMAT_XRGB8888);

    if (!buffer)
    {
        fprintf(stderr,
                "No se pudo crear wl_buffer\n");
        return 1;
    }
    InitWindow(WIDTH, HEIGHT, "WAYMIX");
    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);

        DrawText(
            "ZARAMAGAOS",
            280,
            280,
            30,
            GREEN);

        EndDrawing();

        rlCopyFramebuffer(
            0,
            0,
            WIDTH,
            HEIGHT,
            RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            pixels);

        wl_surface_attach(
            surface,
            buffer,
            0,
            0);

        wl_surface_damage_buffer(
            surface,
            0,
            0,
            WIDTH,
            HEIGHT);

        wl_surface_commit(surface);

        wl_display_flush(display);
    }
    /* -----------------------------------------
     * Mandar framebuffer a Wayland
     * ----------------------------------------- */

    // wl_surface_attach(
    //     surface,
    //     buffer,
    //     0,
    //     0);

    // wl_surface_damage_buffer(
    //     surface,
    //     0,
    //     0,
    //     WIDTH,
    //     HEIGHT);

    // wl_surface_commit(surface);

    // /* -----------------------------------------
    //  * Esperar procesamiento
    //  * ----------------------------------------- */

    // wl_display_roundtrip(display);

    // printf(
    //     "Buffer enviado a Wayland: %p\n",
    //     pixels);

    // printf(
    //     "SIZE: %d bytes\n",
    //     SIZE);

    // /* -----------------------------------------
    //  * Mantener ventana viva
    //  * ----------------------------------------- */

    // while (wl_display_dispatch(display) != -1)
    // {
    // }

    return 0;
}