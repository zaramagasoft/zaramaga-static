#include <wayland-client.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH  800
#define HEIGHT 600
#define SIZE   (WIDTH * HEIGHT * 4)

static unsigned char framebuffer[SIZE];

static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct wl_surface *surface;
static struct wl_shm_pool *pool;
static struct wl_buffer *buffer;

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
}

static void registry_remove(
    void *data,
    struct wl_registry *registry,
    uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    registry_global,
    registry_remove
};

int main(void)
{
    printf("Wayland 800x600\n");

    /* Conectar con Wayland */
    display = wl_display_connect(NULL);

    if (!display)
    {
        fprintf(stderr, "No se pudo conectar a Wayland\n");
        return 1;
    }

    /* Obtener compositor y SHM */
    struct wl_registry *registry =
        wl_display_get_registry(display);

    wl_registry_add_listener(
        registry,
        &registry_listener,
        NULL);

    wl_display_roundtrip(display);

    if (!compositor || !shm)
    {
        fprintf(stderr, "No tenemos compositor o SHM\n");
        return 1;
    }

    /* Crear superficie */
    surface = wl_compositor_create_surface(compositor);

    /*
     * Para la primera prueba usamos /dev/shm.
     */
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
        return 1;
    }

    /*
     * Mapear memoria SHM.
     */
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
        return 1;
    }

    /*
     * Poner fondo negro.
     */
    memset(pixels, 0, SIZE);

    /*
     * Pintar un rectángulo verde.
     *
     * Formato:
     * XRGB8888
     */
    for (int y = 200; y < 400; y++)
    {
        for (int x = 250; x < 550; x++)
        {
            unsigned char *p =
                pixels + (y * WIDTH + x) * 4;

            p[0] = 0;     // B
            p[1] = 255;   // G
            p[2] = 0;     // R
            p[3] = 255;   // X
        }
    }

    /*
     * Crear pool Wayland sobre nuestra memoria.
     */
    pool = wl_shm_create_pool(
        shm,
        fd,
        SIZE);

    buffer = wl_shm_pool_create_buffer(
        pool,
        0,
        WIDTH,
        HEIGHT,
        WIDTH * 4,
        WL_SHM_FORMAT_XRGB8888);

    /*
     * Mandar buffer a Wayland.
     */
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

    wl_display_roundtrip(display);

    printf("Buffer enviado a Wayland: %p\n", pixels);
    printf("SIZE: %d bytes\n", SIZE);

    /*
     * Mantener la ventana viva.
     */
    while (wl_display_dispatch(display) != -1)
    {
    }

    return 0;
}