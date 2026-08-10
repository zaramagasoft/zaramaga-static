#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include <poll.h>

/* DRM primero */
#include <drm/drm_fourcc.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

/* Input después */
#include "zinput.h"

#define MAX_OUTPUTS 16
#define CURSOR_SIZE 8
#define DRM_BUFFERS 2

static int mouse_x = 0;
static int mouse_y = 0;

typedef struct
{
    uint32_t fb_id;

    struct drm_mode_create_dumb create;
    struct drm_mode_map_dumb map;

    uint32_t *pixels;

} DrmBuffer;

typedef struct
{
    int fd;

    uint32_t connector_id;
    uint32_t encoder_id;
    uint32_t crtc_id;

    drmModeModeInfo mode;
    drmModeCrtc *old_crtc;

    /* Framebuffer */
    uint32_t fb_id;

    struct drm_mode_create_dumb create;
    struct drm_mode_map_dumb map;

    uint32_t *pixels;
    DrmBuffer buffers[DRM_BUFFERS];

    int front;
    int back;

} DrmOutput;

typedef struct
{
    DrmOutput outputs[MAX_OUTPUTS];
    int count;

} DrmSystem;
#define MAX_INPUTS 32

int input_fds[MAX_INPUTS];
int input_count = 0;

void draw_cursor(DrmOutput *output)
{
    DrmBuffer *buffer =
        &output->buffers[output->back];
    uint32_t pitch = buffer->create.pitch / 4;
    uint32_t color = 0x00FF0000;

    for (int i = -CURSOR_SIZE; i <= CURSOR_SIZE; i++)
    {
        int x = mouse_x + i;
        int y = mouse_y;

        if (x >= 0 &&
            x < (int)buffer->create.width)
        {
            buffer->pixels[y * pitch + x] = color;
        }

        x = mouse_x;
        y = mouse_y + i;

        if (y >= 0 &&
            y < (int)buffer->create.height)
        {
            buffer->pixels[y * pitch + x] = color;
        }
    }
}
void test_input(void)
{
    char path[64];
    int fds[32];
    int count = 0;

    for (int i = 0; i < 32; i++)
    {
        snprintf(path, sizeof(path),
                 "/dev/input/event%d", i);

        int fd = open(path, O_RDONLY | O_NONBLOCK);

        if (fd < 0)
            continue;

        fds[count++] = fd;

        printf("INPUT %s fd=%d\n", path, fd);
    }

    struct input_event ev;

    while (1)
    {
        for (int i = 0; i < count; i++)
        {
            while (read(fds[i], &ev, sizeof(ev)) == sizeof(ev))
            {
                printf("fd=%d  type=%u code=%u value=%d\n",
                       fds[i],
                       ev.type,
                       ev.code,
                       ev.value);
            }
        }

        usleep(10000);
    }
}

int open_inputs(void)
{
    char path[64];

    input_count = 0;

    for (int i = 0; i < MAX_INPUTS; i++)
    {
        snprintf(
            path,
            sizeof(path),
            "/dev/input/event%d",
            i);

        int fd = open(
            path,
            O_RDONLY | O_NONBLOCK);

        if (fd < 0)
            continue;

        input_fds[input_count++] = fd;

        printf(
            "INPUT: %s fd=%d\n",
            path,
            fd);

        if (input_count >= MAX_INPUTS)
            break;
    }

    return input_count;
}

/// open mouse device //////
int open_mouse(void)
{
    char path[64];

    for (int i = 0; i < 32; i++)
    {
        snprintf(
            path,
            sizeof(path),
            "/dev/input/event%d",
            i);

        int fd = open(
            path,
            O_RDONLY | O_NONBLOCK);

        if (fd < 0)
            continue;

        printf("INPUT: %s\n", path);

        return fd;
    }

    return -1;
}
void process_inputs(DrmOutput *output)
{
    struct input_event ev;

    for (int i = 0; i < input_count; i++)
    {
        while (read(input_fds[i], &ev, sizeof(ev)) == sizeof(ev))
        {
            if (ev.type != EV_REL)
                continue;

            if (ev.code == REL_X)
                mouse_x += ev.value;

            else if (ev.code == REL_Y)
                mouse_y += ev.value;
        }
    }

    uint32_t width =
        output->buffers[0].create.width;

    uint32_t height =
        output->buffers[0].create.height;

    if (mouse_x < 0)
        mouse_x = 0;

    if (mouse_y < 0)
        mouse_y = 0;

    if (mouse_x >= (int)width)
        mouse_x = width - 1;

    if (mouse_y >= (int)height)
        mouse_y = height - 1;
}
void process_mouse(int fd, DrmOutput *output)
{
    struct input_event ev;

    while (read(fd, &ev, sizeof(ev)) == sizeof(ev))
    {
        if (ev.type == EV_REL)
        {
            if (ev.code == REL_X)
                mouse_x += ev.value;

            if (ev.code == REL_Y)
                mouse_y += ev.value;
        }
    }

    /*
     * Limitar cursor a pantalla
     */

    if (mouse_x < 0)
        mouse_x = 0;

    if (mouse_y < 0)
        mouse_y = 0;

    if (mouse_x >= (int)output->create.width)
        mouse_x = output->create.width - 1;

    if (mouse_y >= (int)output->create.height)
        mouse_y = output->create.height - 1;
}

/* ============================================================
 * LIMPIAR
 * ============================================================ */

int drm_cleanup(DrmOutput *output)
{
    /* Restaurar CRTC original */
    if (output->old_crtc && output->fd >= 0)
    {
        drmModeSetCrtc(
            output->fd,
            output->old_crtc->crtc_id,
            output->old_crtc->buffer_id,
            output->old_crtc->x,
            output->old_crtc->y,
            &output->connector_id,
            1,
            &output->old_crtc->mode);

        drmModeFreeCrtc(output->old_crtc);
        output->old_crtc = NULL;
    }

    /* Liberar los dos buffers */
    for (int i = 0; i < DRM_BUFFERS; i++)
    {
        DrmBuffer *buffer =
            &output->buffers[i];

        /* Eliminar framebuffer DRM */
        if (buffer->fb_id &&
            output->fd >= 0)
        {
            drmModeRmFB(
                output->fd,
                buffer->fb_id);

            buffer->fb_id = 0;
        }

        /* Desmapear memoria */
        if (buffer->pixels)
        {
            munmap(
                buffer->pixels,
                buffer->create.size);

            buffer->pixels = NULL;
        }

        /* Destruir dumb buffer */
        if (buffer->create.handle &&
            output->fd >= 0)
        {
            struct drm_mode_destroy_dumb destroy =
                {0};

            destroy.handle =
                buffer->create.handle;

            if (drmIoctl(
                    output->fd,
                    DRM_IOCTL_MODE_DESTROY_DUMB,
                    &destroy) < 0)
            {
                perror(
                    "DRM_IOCTL_MODE_DESTROY_DUMB");
            }

            buffer->create.handle = 0;
        }
    }

    /* Cerrar DRM */
    if (output->fd >= 0)
    {
        drmDropMaster(output->fd);

        close(output->fd);

        output->fd = -1;
    }

    return 0;
}

/* ============================================================
 * CREAR FRAMEBUFFER
 * ============================================================ */

int drm_create_buffer(DrmOutput *output, DrmBuffer *buffer)
{
    buffer->create.width =
        output->mode.hdisplay;

    buffer->create.height =
        output->mode.vdisplay;

    buffer->create.bpp = 32;

    if (drmIoctl(
            output->fd,
            DRM_IOCTL_MODE_CREATE_DUMB,
            &buffer->create) < 0)
    {
        perror("CREATE_DUMB");
        return -1;
    }

    buffer->map.handle =
        buffer->create.handle;

    if (drmIoctl(
            output->fd,
            DRM_IOCTL_MODE_MAP_DUMB,
            &buffer->map) < 0)
    {
        perror("MAP_DUMB");
        return -1;
    }

    buffer->pixels = mmap(
        NULL,
        buffer->create.size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        output->fd,
        buffer->map.offset);

    if (buffer->pixels == MAP_FAILED)
    {
        perror("mmap");

        buffer->pixels = NULL;

        return -1;
    }

    uint32_t handles[4] = {0};
    uint32_t pitches[4] = {0};
    uint32_t offsets[4] = {0};

    handles[0] =
        buffer->create.handle;

    pitches[0] =
        buffer->create.pitch;

    if (drmModeAddFB2(
            output->fd,
            buffer->create.width,
            buffer->create.height,
            DRM_FORMAT_XRGB8888,
            handles,
            pitches,
            offsets,
            &buffer->fb_id,
            0) != 0)
    {
        perror("drmModeAddFB2");
        return -1;
    }

    return 0;
}
int drm_create_framebuffers(DrmOutput *output)
{
    for (int i = 0; i < DRM_BUFFERS; i++)
    {
        if (drm_create_buffer(
                output,
                &output->buffers[i]) < 0)
        {
            return -1;
        }
    }

    output->front = 0;
    output->back = 1;

    return 0;
}
/* ============================================================
 * BUSCAR SALIDAS
 * ============================================================ */

int drm_find_outputs(DrmSystem *system)
{
    DIR *dir;
    struct dirent *entry;

    system->count = 0;

    dir = opendir("/dev/dri");

    if (!dir)
    {
        perror("opendir /dev/dri");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        /*
         * Solo card0, card1...
         */

        if (strncmp(
                entry->d_name,
                "card",
                4) != 0)
        {
            continue;
        }

        char path[64];

        snprintf(
            path,
            sizeof(path),
            "/dev/dri/%s",
            entry->d_name);

        int fd = open(
            path,
            O_RDWR);

        if (fd < 0)
            continue;

        printf(
            "\nDRM: %s\n",
            path);

        drmModeRes *resources =
            drmModeGetResources(fd);

        if (!resources)
        {
            close(fd);
            continue;
        }

        /*
         * Buscar conectores
         */

        for (int i = 0;
             i < resources->count_connectors;
             i++)
        {
            uint32_t connector_id =
                resources->connectors[i];

            drmModeConnector *conn =
                drmModeGetConnector(
                    fd,
                    connector_id);

            if (!conn)
                continue;

            if (conn->connection !=
                DRM_MODE_CONNECTED)
            {
                drmModeFreeConnector(conn);
                continue;
            }

            if (conn->count_modes == 0)
            {
                drmModeFreeConnector(conn);
                continue;
            }

            if (system->count >= MAX_OUTPUTS)
            {
                drmModeFreeConnector(conn);
                break;
            }

            DrmOutput *output =
                &system->outputs[system->count];

            memset(
                output,
                0,
                sizeof(DrmOutput));

            output->fd = dup(fd);

            output->connector_id =
                conn->connector_id;

            /*
             * Primer modo.
             */

            output->mode =
                conn->modes[0];

            /*
             * Encoder
             */

            drmModeEncoder *encoder = NULL;

            if (conn->encoder_id)
            {
                encoder =
                    drmModeGetEncoder(
                        fd,
                        conn->encoder_id);
            }

            if (!encoder)
            {
                drmModeFreeConnector(conn);
                continue;
            }

            output->encoder_id =
                encoder->encoder_id;

            output->crtc_id =
                encoder->crtc_id;

            /*
             * Guardar CRTC original
             */

            output->old_crtc =
                drmModeGetCrtc(
                    fd,
                    output->crtc_id);

            if (!output->old_crtc)
            {
                drmModeFreeEncoder(encoder);
                drmModeFreeConnector(conn);
                continue;
            }

            printf(
                "  Connector: %u\n",
                output->connector_id);

            printf(
                "  Encoder:   %u\n",
                output->encoder_id);

            printf(
                "  CRTC:      %u\n",
                output->crtc_id);

            printf(
                "  Modo:      %ux%u @ %u Hz\n",
                output->mode.hdisplay,
                output->mode.vdisplay,
                output->mode.vrefresh);

            system->count++;

            drmModeFreeEncoder(encoder);
            drmModeFreeConnector(conn);
        }

        drmModeFreeResources(resources);

        close(fd);
    }

    closedir(dir);

    printf(
        "\nSalidas DRM: %d\n",
        system->count);

    return system->count;
}

/* ============================================================
 * PRESENTAR
 * ============================================================ */

int drm_present(DrmOutput *output)
{
    DrmBuffer *buffer =
        &output->buffers[output->back];

    uint32_t connector_id =
        output->connector_id;

    int ret = drmModeSetCrtc(
        output->fd,
        output->crtc_id,
        buffer->fb_id,
        0,
        0,
        &connector_id,
        1,
        &output->mode);

    if (ret != 0)
    {
        perror("drmModeSetCrtc");
        return -1;
    }

    /* El que acabamos de presentar pasa a ser FRONT */
    int old_front = output->front;

    output->front = output->back;
    output->back = old_front;

    return 0;
}

/* ============================================================
 * DIBUJAR
 * ============================================================ */

void draw_test(DrmOutput *output)
{
    DrmBuffer *buffer =
        &output->buffers[output->back];
    uint32_t width =
        buffer->create.width;

    uint32_t height =
        buffer->create.height;

    uint32_t pitch =
        buffer->create.pitch / 4;

    /*
     * Negro completo
     */

    for (uint32_t y = 0;
         y < height;
         y++)
    {
        uint32_t *row =
            buffer->pixels +
            y * pitch;

        for (uint32_t x = 0;
             x < width;
             x++)
        {
            row[x] = 0x00000000;
        }
    }

    /*
     * Cuadrado rojo
     */

    const uint32_t size = 100;

    uint32_t x0 =
        (width - size) / 2;

    uint32_t y0 =
        (height - size) / 2;

    for (uint32_t y = 0;
         y < size;
         y++)
    {
        uint32_t *row =
            buffer->pixels +
            (y0 + y) * pitch;

        for (uint32_t x = 0;
             x < size;
             x++)
        {
            row[x0 + x] =
                0x00FF0000;
        }
    }
}
/* ============================================================
 * CURSOR
 * ============================================================ */

/* ============================================================
 * MAIN
 * ============================================================ */

int main(void)
{
    DrmSystem systema = {0};

    if (drm_find_outputs(&systema) <= 0)
    {
        fprintf(
            stderr,
            "No se encontraron salidas DRM\n");

        return EXIT_FAILURE;
    }

    /*
     * Por ahora solamente
     * trabajamos con la primera salida.
     */

    DrmOutput *output =
        &systema.outputs[0];

    /*
     * Crear los dos framebuffers.
     */

    if (drm_create_framebuffers(output) < 0)
    {
        drm_cleanup(output);
        return EXIT_FAILURE;
    }

    /*
     * Posición inicial del ratón.
     */

    mouse_x =
        output->buffers[0].create.width / 2;

    mouse_y =
        output->buffers[0].create.height / 2;

    draw_test(output);
    draw_cursor(output);

    printf("Presentando buffer %d\n", output->back);

    if (drm_present(output) < 0)
    {
        drm_cleanup(output);
        return EXIT_FAILURE;
    }

    printf("Ahora FRONT=%d BACK=%d\n",
           output->front,
           output->back);

    sleep(5);
    return EXIT_SUCCESS;
}