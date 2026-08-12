#define _GNU_SOURCE
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#define DP_CONNECTOR 826
#define HDMI_CONNECTOR 830


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

#define DEVICE "/dev/dri/card1"

#define WIDTH 1920
#define HEIGHT 1080

#define DP_CONNECTOR 826
#define DP_CRTC 200

#define HDMI_CONNECTOR 830
#define HDMI_CRTC 392

typedef struct
{
    int fd;

    uint32_t handle;
    uint32_t pitch;
    uint64_t size;

    uint32_t fb_id;

    void *map;

    drmModeCrtc *dp_old;
    drmModeCrtc *hdmi_old;

} DrmClone;

static void cleanup(DrmClone *d)
{
    if (!d)
        return;

    if (d->fd >= 0)
    {
        uint32_t dp_connector = DP_CONNECTOR;
        uint32_t hdmi_connector = HDMI_CONNECTOR;

        if (d->dp_old)
        {
            drmModeSetCrtc(
                d->fd,
                d->dp_old->crtc_id,
                d->dp_old->buffer_id,
                d->dp_old->x,
                d->dp_old->y,
                &dp_connector,
                1,
                &d->dp_old->mode);
        }

        if (d->hdmi_old)
        {
            drmModeSetCrtc(
                d->fd,
                d->hdmi_old->crtc_id,
                d->hdmi_old->buffer_id,
                d->hdmi_old->x,
                d->hdmi_old->y,
                &hdmi_connector,
                1,
                &d->hdmi_old->mode);
        }

        if (d->fb_id)
            drmModeRmFB(d->fd, d->fb_id);

        if (d->map && d->map != MAP_FAILED)
            munmap(d->map, d->size);

        if (d->handle)
        {
            struct drm_mode_destroy_dumb destroy = {
                .handle = d->handle
            };

            ioctl(
                d->fd,
                DRM_IOCTL_MODE_DESTROY_DUMB,
                &destroy);
        }

        close(d->fd);
        d->fd = -1;
    }
}

static int create_dumb_buffer(DrmClone *d)
{
    struct drm_mode_create_dumb create = {0};

    create.width = WIDTH;
    create.height = HEIGHT;
    create.bpp = 32;

    if (ioctl(
            d->fd,
            DRM_IOCTL_MODE_CREATE_DUMB,
            &create) < 0)
    {

        perror("CREATE_DUMB");
        return -1;
    }

    d->handle = create.handle;
    d->pitch = create.pitch;
    d->size = create.size;

    printf("DUMB BUFFER\n");
    printf("  handle : %u\n", d->handle);
    printf("  pitch  : %u\n", d->pitch);
    printf("  size   : %llu\n",
           (unsigned long long)d->size);

    struct drm_mode_map_dumb map = {
        .handle = d->handle};

    if (ioctl(
            d->fd,
            DRM_IOCTL_MODE_MAP_DUMB,
            &map) < 0)
    {

        perror("MAP_DUMB");
        return -1;
    }

    d->map = mmap(
        NULL,
        d->size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        d->fd,
        map.offset);

    if (d->map == MAP_FAILED)
    {
        perror("mmap");
        d->map = NULL;
        return -1;
    }

    return 0;
}

static int create_framebuffer(DrmClone *d)
{
    uint32_t handles[4] = {
        d->handle,
        0,
        0,
        0};

    uint32_t pitches[4] = {
        d->pitch,
        0,
        0,
        0};

    uint32_t offsets[4] = {
        0,
        0,
        0,
        0};

    if (drmModeAddFB2(
            d->fd,
            WIDTH,
            HEIGHT,
            DRM_FORMAT_XRGB8888,
            handles,
            pitches,
            offsets,
            &d->fb_id,
            0) < 0)
    {

        perror("drmModeAddFB2");
        return -1;
    }

    printf("\nFRAMEBUFFER\n");
    printf("  FB ID  : %u\n", d->fb_id);
    printf("  format : XRGB8888\n");
    printf("  size   : %dx%d\n", WIDTH, HEIGHT);

    return 0;
}

static void paint_buffer(DrmClone *d)
{
    uint32_t *pixels = d->map;

    for (int y = 0; y < HEIGHT; y++)
    {

        for (int x = 0; x < WIDTH; x++)
        {

            /*
             * Verde ZaramagaOS.
             *
             * XRGB8888:
             *
             * 0x00RRGGBB
             */

            uint8_t r = 0;
            uint8_t g = 180;
            uint8_t b = 0;

            pixels[y * (d->pitch / 4) + x] =
                ((uint32_t)r << 16) |
                ((uint32_t)g << 8) |
                b;
        }
    }
}

static int modeset_output(
    DrmClone *d,
    uint32_t connector_id,
    uint32_t crtc_id,
    drmModeCrtc **old_crtc)
{
    drmModeConnector *conn =
        drmModeGetConnector(d->fd, connector_id);

    if (!conn)
    {
        fprintf(stderr,
                "No se pudo obtener connector %u\n",
                connector_id);
        return -1;
    }

    if (conn->connection != DRM_MODE_CONNECTED)
    {

        fprintf(stderr,
                "Connector %u no conectado\n",
                connector_id);

        drmModeFreeConnector(conn);
        return -1;
    }

    drmModeModeInfo mode;
    int found = 0;

    memset(&mode, 0, sizeof(mode));

    for (int i = 0; i < conn->count_modes; i++)
    {

        if (conn->modes[i].hdisplay == WIDTH &&
            conn->modes[i].vdisplay == HEIGHT &&
            conn->modes[i].vrefresh == 60)
        {

            mode = conn->modes[i];
            found = 1;
            break;
        }
        
    }
   if (!found) {
    fprintf(stderr,
            "No existe %dx%d@60 en connector %u\n",
            WIDTH,
            HEIGHT,
            connector_id);

    drmModeFreeConnector(conn);
    return -1;
}

    *old_crtc =
        drmModeGetCrtc(d->fd, crtc_id);

    if (!*old_crtc)
    {

        perror("drmModeGetCrtc");

        drmModeFreeConnector(conn);
        return -1;
    }

    printf("\nMODESET\n");
    printf("  connector : %u\n", connector_id);
    printf("  CRTC      : %u\n", crtc_id);
    printf("  mode      : %s\n", mode.name);

    int ret = drmModeSetCrtc(
        d->fd,
        crtc_id,
        d->fb_id,
        0,
        0,
        &connector_id,
        1,
        &mode);

    if (ret < 0)
    {

        fprintf(stderr,
                "drmModeSetCrtc(%u) fallo: %s\n",
                connector_id,
                strerror(errno));

        drmModeFreeConnector(conn);

        return -1;
    }

    drmModeFreeConnector(conn);

    return 0;
}

int main(void)
{
    DrmClone d = {0};

    d.fd = -1;

    d.fd = open(
        DEVICE,
        O_RDWR | O_CLOEXEC);

    if (d.fd < 0)
    {
        perror("open /dev/dri/card1");
        return EXIT_FAILURE;
    }

    printf("====================================\n");
    printf("       ZARAMAGA DRM CLONE\n");
    printf("====================================\n");

    printf("\nNVIDIA DRM: %s\n", DEVICE);

    if (create_dumb_buffer(&d) < 0)
        goto fail;

    if (create_framebuffer(&d) < 0)
        goto fail;

    paint_buffer(&d);

    /*
     * Primero DP.
     */
    if (modeset_output(
            &d,
            DP_CONNECTOR,
            DP_CRTC,
            &d.dp_old) < 0)
        goto fail;

    /*
     * Después HDMI.
     */
    if (modeset_output(
            &d,
            HDMI_CONNECTOR,
            HDMI_CRTC,
            &d.hdmi_old) < 0)
        goto fail;

    printf("\n");
    printf("====================================\n");
    printf("        CLONE ACTIVO\n");
    printf("====================================\n");
    printf("\n");
    printf("DP   : 1920x1080 @ 60\n");
    printf("HDMI : 1920x1080 @ 60\n");
    printf("\n");
    printf("Pulsa ENTER para restaurar...\n");

    getchar();

    cleanup(&d);

    return EXIT_SUCCESS;

fail:

    cleanup(&d);

    return EXIT_FAILURE;
}