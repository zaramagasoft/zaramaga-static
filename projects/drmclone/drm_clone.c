#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

int main(void)
{
    const char *device = "/dev/dri/card1";

    int fd = open(device, O_RDWR | O_CLOEXEC);

    if (fd < 0)
    {
        perror("open DRM");
        return EXIT_FAILURE;
    }

    drmModeRes *res = drmModeGetResources(fd);

    if (!res)
    {
        perror("drmModeGetResources");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("====================================\n");
    printf("       NVIDIA DRM CLONE TEST\n");
    printf("====================================\n\n");

    printf("CRTCs: %d\n", res->count_crtcs);

    for (int i = 0; i < res->count_crtcs; i++)
    {

        drmModeCrtc *crtc =
            drmModeGetCrtc(fd, res->crtcs[i]);

        if (!crtc)
            continue;

        printf("  CRTC %u\n", crtc->crtc_id);

        drmModeFreeCrtc(crtc);
    }

    printf("\nConnectors: %d\n", res->count_connectors);

    for (int i = 0; i < res->count_connectors; i++)
    {

        drmModeConnector *conn =
            drmModeGetConnector(fd, res->connectors[i]);

        if (!conn)
            continue;

        printf("\n------------------------------------\n");

        printf("Connector ID : %u\n",
               conn->connector_id);

        printf("Type         : %s\n",
               drmModeGetConnectorTypeName(conn->connector_type));

        printf("Status       : %s\n",
               conn->connection == DRM_MODE_CONNECTED
                   ? "CONNECTED"
                   : "DISCONNECTED");

        printf("Encoder ID   : %u\n",
               conn->encoder_id);
        if (conn->encoder_id)
        {

            drmModeEncoder *enc =
                drmModeGetEncoder(fd, conn->encoder_id);

            if (enc)
            {

                printf("Possible CRTCs: 0x%x\n",
                       enc->possible_crtcs);

                printf("Current CRTC  : %u\n",
                       enc->crtc_id);

                drmModeFreeEncoder(enc);
            }
        }
        printf("Modes        : %d\n",
               conn->count_modes);

        for (int m = 0; m < conn->count_modes; m++)
        {

            drmModeModeInfo *mode =
                &conn->modes[m];

            printf("    %s @ %d Hz\n",
                   mode->name,
                   mode->vrefresh);
        }
        drmModeEncoder *enc =
            drmModeGetEncoder(fd, conn->encoder_id);

        if (enc)
        {
            printf("  CRTC actual   : %u\n",
                   enc->crtc_id);

            printf("  possible CRTCs: 0x%x\n",
                   enc->possible_crtcs);

            drmModeFreeEncoder(enc);
        }

        drmModeFreeConnector(conn);
    }
    drmModePlaneRes *pres = drmModeGetPlaneResources(fd);

    if (!pres)
    {
        perror("drmModeGetPlaneResources");
    }
    else
    {

        printf("\n====================================\n");
        printf("              PLANES\n");
        printf("====================================\n");

        printf("Planes: %u\n", pres->count_planes);

        for (uint32_t i = 0; i < pres->count_planes; i++)
        {

            drmModePlane *plane =
                drmModeGetPlane(fd, pres->planes[i]);

            if (!plane)
                continue;

            printf("\nPlane %u\n", plane->plane_id);

            printf("  possible CRTCs: 0x%x\n",
                   plane->possible_crtcs);

            printf("  formats: ");

            for (uint32_t f = 0; f < plane->count_formats; f++)
            {

                printf("%c%c%c%c ",
                       plane->formats[f] & 0xff,
                       (plane->formats[f] >> 8) & 0xff,
                       (plane->formats[f] >> 16) & 0xff,
                       (plane->formats[f] >> 24) & 0xff);
            }

            printf("\n");

            drmModeFreePlane(plane);
        }

        drmModeFreePlaneResources(pres);
    }
    drmModeFreeResources(res);
    close(fd);

    return EXIT_SUCCESS;
}