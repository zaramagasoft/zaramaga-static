#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// API DRM
#include <cairo/cairo.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// Fuentes y logo
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H

//#include "logo_svg.h"

drmModeModeInfo modez;

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  float x, y;
} Vec2;

static int g_width = 1920;
static int g_height = 1080;

static Vec3 cube_nodes[8] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
                             {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}};

static int cube_edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};



int main(int argc, char const *argv[])
{
    int fd = -1;

    /*
     * Buscar todas las tarjetas DRM disponibles.
     * Probamos card0, card1, ... card15.
     */
    for (int card = 0; card < 16; card++)
    {
        char path[64];

        snprintf(path, sizeof(path), "/dev/dri/card%d", card);

        fd = open(path, O_RDWR);

        if (fd < 0)
            continue;

        printf("\n");
        printf("========================================\n");
        printf(" DRM DEVICE: %s\n", path);
        printf("========================================\n");

        drmModeRes *resources = drmModeGetResources(fd);

        if (!resources)
        {
            perror("drmModeGetResources");
            close(fd);
            fd = -1;
            continue;
        }

        printf("Connectors: %d\n", resources->count_connectors);
        printf("Encoders:   %d\n", resources->count_encoders);
        printf("CRTCs:      %d\n", resources->count_crtcs);
        printf("\n");

        /*
         * Enumerar TODOS los conectores
         */
        for (int i = 0; i < resources->count_connectors; i++)
        {
            uint32_t connector_id = resources->connectors[i];

            drmModeConnector *conn =
                drmModeGetConnector(fd, connector_id);

            if (!conn)
                continue;

            printf("----------------------------------------\n");
            printf("Connector ID : %u\n", conn->connector_id);
            printf("Encoder ID   : %u\n", conn->encoder_id);

            printf("Tipo         : ");

            switch (conn->connector_type)
            {
                case DRM_MODE_CONNECTOR_HDMIA:
                    printf("HDMI-A\n");
                    break;

                case DRM_MODE_CONNECTOR_HDMIB:
                    printf("HDMI-B\n");
                    break;

                case DRM_MODE_CONNECTOR_DisplayPort:
                    printf("DisplayPort\n");
                    break;

                case DRM_MODE_CONNECTOR_DVID:
                    printf("DVI-D\n");
                    break;

                case DRM_MODE_CONNECTOR_DVII:
                    printf("DVI-I\n");
                    break;

                case DRM_MODE_CONNECTOR_VGA:
                    printf("VGA\n");
                    break;

                default:
                    printf("Tipo DRM %u\n",
                           conn->connector_type);
                    break;
            }

            printf("Estado       : ");

            if (conn->connection == DRM_MODE_CONNECTED)
                printf("CONNECTED\n");
            else if (conn->connection == DRM_MODE_DISCONNECTED)
                printf("DISCONNECTED\n");
            else
                printf("UNKNOWN\n");

            printf("Modos        : %d\n", conn->count_modes);

            /*
             * Mostrar todos los modos disponibles
             */
            for (int j = 0; j < conn->count_modes; j++)
            {
                drmModeModeInfo *mode = &conn->modes[j];

                printf("  %4dx%-4d @ %-3d Hz",
                       mode->hdisplay,
                       mode->vdisplay,
                       mode->vrefresh);

                if (mode->type & DRM_MODE_TYPE_PREFERRED)
                    printf("  [PREFERRED]");

                printf("\n");
            }

            /*
             * Si está conectado, mostrar encoder y CRTC
             */
            if (conn->connection == DRM_MODE_CONNECTED)
            {
                printf("\n");

                if (conn->encoder_id)
                {
                    drmModeEncoder *encoder =
                        drmModeGetEncoder(fd, conn->encoder_id);

                    if (encoder)
                    {
                        printf("Encoder %u\n",
                               encoder->encoder_id);

                        printf("  CRTC actual : %u\n",
                               encoder->crtc_id);

                        printf("  CRTC posible: 0x%08x\n",
                               encoder->possible_crtcs);

                        drmModeFreeEncoder(encoder);
                    }
                }
            }

            drmModeFreeConnector(conn);
        }

        printf("----------------------------------------\n");

        drmModeFreeResources(resources);

        close(fd);
        fd = -1;
    }

    printf("\n");
    printf("========================================\n");
    printf(" FIN DETECCION DRM\n");
    printf("========================================\n");
    sleep(9);
    return 0;
}