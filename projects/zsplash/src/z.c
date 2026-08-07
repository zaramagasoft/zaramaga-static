#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
// api DRM
#include <cairo/cairo.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <stdarg.h>
#include <dirent.h>

// fuentes y logo
#include <cairo/cairo-ft.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "font3270.h"

#include "parser.h"
#include "paths.h"

#define MAX_OUTPUTS 16

drmModeModeInfo modez;

drmModeCrtc *crtcz;

int drm_fd = -1;

drmModeRes *resources = NULL;
drmModeConnector *connector = NULL;
drmModeEncoder *encoder = NULL;
drmModeCrtc *crtc = NULL;

uint32_t fb_id = 0;

struct drm_mode_create_dumb create = {0};
struct drm_mode_map_dumb map = {0};

uint8_t *fb_data = NULL;
size_t fb_size = 0;

cairo_surface_t *surface = NULL;
cairo_t *cr = NULL;
typedef struct
{
    float x, y, z;
} Vec3;
typedef struct
{
    float x, y;
} Vec2;
static int g_width = 800;
static int g_height = 600;
/// Carga la fuente ////
FT_Library ft;
FT_Face face;
/// drmdrives ///
typedef struct
{
    int fd;

    uint32_t connector_id;
    uint32_t encoder_id;
    uint32_t crtc_id;

    drmModeModeInfo mode;

    /* Framebuffer */
    uint32_t fb_id;

    struct drm_mode_create_dumb create;
    struct drm_mode_map_dumb map;

    uint32_t *pixels;

    cairo_surface_t *surface;
    cairo_t *cr;
} DrmOutput;

typedef struct
{
    DrmOutput outputs[MAX_OUTPUTS];
    int count;
} DrmSystem;

int drm_find_outputs(DrmSystem *system);
int drm_create_framebuffer(DrmOutput *output);
int drm_present(DrmOutput *output);
int drm_create_surface(DrmOutput *output);
int drm_draw_splash(DrmOutput *output, FT_Face *face);
int draw_logo(cairo_t *cr);
int draw_logo(cairo_t *cr)
{
    char *paths[] = {
        path1,
        path2,
        path3,
        path4,
        path5,
        path6,
        path7,
        path8,
        path9
    };

    int num_paths = sizeof(paths) / sizeof(paths[0]);

    for (int i = 0; i < num_paths; i++)
    {
        parse_svg_path(cr, paths[i]);
        cairo_stroke(cr);
    }

    return 0;
}
int drm_draw_splash(DrmOutput *output, FT_Face *face)
{
   cairo_t *cr = output->cr;

    /* Fondo */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    /* Logo */
    cairo_set_source_rgb(cr, 0.2, 1.0, 0.2);

    cairo_save(cr);

    cairo_translate(cr, 200, 150);
    cairo_scale(cr, 2.0, 2.0);

    cairo_set_line_width(cr, 1.0);

    draw_logo(cr);

    cairo_restore(cr);

    cairo_surface_flush(output->surface);

    return 0;
}
int drm_create_surface(DrmOutput *output)
{
    output->surface = cairo_image_surface_create_for_data(
        (unsigned char *)output->pixels,
        CAIRO_FORMAT_RGB24,
        output->create.width,
        output->create.height,
        output->create.pitch);

    if (cairo_surface_status(output->surface) != CAIRO_STATUS_SUCCESS)
    {
        fprintf(stderr,
                "Error creando superficie Cairo: %s\n",
                cairo_status_to_string(
                    cairo_surface_status(output->surface)));

        cairo_surface_destroy(output->surface);
        output->surface = NULL;

        return -1;
    }

    output->cr = cairo_create(output->surface);

    if (cairo_status(output->cr) != CAIRO_STATUS_SUCCESS)
    {
        fprintf(stderr,
                "Error creando contexto Cairo: %s\n",
                cairo_status_to_string(cairo_status(output->cr)));

        cairo_destroy(output->cr);
        cairo_surface_destroy(output->surface);

        output->cr = NULL;
        output->surface = NULL;

        return -1;
    }

    return 0;
}
int drm_create_framebuffer(DrmOutput *output)
{
    output->create.width = output->mode.hdisplay;
    output->create.height = output->mode.vdisplay;
    output->create.bpp = 32;

    /* Crear dumb buffer */
    if (drmIoctl(output->fd,
                 DRM_IOCTL_MODE_CREATE_DUMB,
                 &output->create) < 0)
    {
        perror("DRM_IOCTL_MODE_CREATE_DUMB");
        return -1;
    }

    printf("Framebuffer:\n");
    printf("  %ux%u\n",
           output->create.width,
           output->create.height);
    printf("  pitch  : %u\n", output->create.pitch);
    printf("  size   : %llu\n",
           (unsigned long long)output->create.size);
    printf("  handle : %u\n",
           output->create.handle);

    /*
     * Mapear dumb buffer
     */
    output->map.handle = output->create.handle;

    if (drmIoctl(output->fd,
                 DRM_IOCTL_MODE_MAP_DUMB,
                 &output->map) < 0)
    {
        perror("DRM_IOCTL_MODE_MAP_DUMB");
        return -1;
    }

    /*
     * Mapear memoria a nuestro espacio de usuario
     */
    output->pixels =
        mmap(NULL,
             output->create.size,
             PROT_READ | PROT_WRITE,
             MAP_SHARED,
             output->fd,
             output->map.offset);

    if (output->pixels == MAP_FAILED)
    {
        perror("mmap");
        output->pixels = NULL;
        return -1;
    }

    /*
     * Crear framebuffer DRM
     */
    uint32_t handles[4] = {0};
    uint32_t pitches[4] = {0};
    uint32_t offsets[4] = {0};

    handles[0] = output->create.handle;
    pitches[0] = output->create.pitch;
    offsets[0] = 0;

    if (drmModeAddFB2(output->fd,
                      output->create.width,
                      output->create.height,
                      DRM_FORMAT_XRGB8888,
                      handles,
                      pitches,
                      offsets,
                      &output->fb_id,
                      0) != 0)
    {
        perror("drmModeAddFB2");
        return -1;
    }

    printf("  fb_id  : %u\n", output->fb_id);

    return 0;
}
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
         * Solo card0, card1, card2...
         */
        if (strncmp(entry->d_name, "card", 4) != 0)
            continue;

        char path[64];

        snprintf(path,
                 sizeof(path),
                 "/dev/dri/%s",
                 entry->d_name);

        int fd = open(path, O_RDWR);

        if (fd < 0)
            continue;

        printf("\nDRM: %s\n", path);

        drmModeRes *resources =
            drmModeGetResources(fd);

        if (!resources)
        {
            close(fd);
            continue;
        }

        /*
         * Buscar todos los conectores
         */
        for (int i = 0;
             i < resources->count_connectors;
             i++)
        {
            uint32_t connector_id =
                resources->connectors[i];

            drmModeConnector *conn =
                drmModeGetConnector(fd, connector_id);

            if (!conn)
                continue;

            printf("  Connector %u: ",
                   conn->connector_id);

            if (conn->connection != DRM_MODE_CONNECTED)
            {
                printf("desconectado\n");

                drmModeFreeConnector(conn);
                continue;
            }

            if (conn->count_modes == 0)
            {
                printf("conectado pero sin modos\n");

                drmModeFreeConnector(conn);
                continue;
            }

            printf("CONECTADO\n");

            /*
             * Hemos encontrado una salida.
             */
            if (system->count >= MAX_OUTPUTS)
            {
                printf("Máximo de salidas alcanzado\n");

                drmModeFreeConnector(conn);
                break;
            }

            DrmOutput *out =
                &system->outputs[system->count];

            memset(out, 0, sizeof(DrmOutput));

            out->fd = fd;

            out->connector_id =
                conn->connector_id;

            /*
             * De momento cogemos el primer modo.
             * Luego hacemos find_best_mode().
             */
            out->mode = conn->modes[0];

            /*
             * Buscar encoder.
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
                printf("    Sin encoder válido\n");

                drmModeFreeConnector(conn);
                continue;
            }

            out->encoder_id =
                encoder->encoder_id;

            out->crtc_id =
                encoder->crtc_id;

            printf("    Encoder: %u\n",
                   out->encoder_id);

            printf("    CRTC: %u\n",
                   out->crtc_id);

            printf("    Modo: %ux%u @ %u Hz\n",
                   out->mode.hdisplay,
                   out->mode.vdisplay,
                   out->mode.vrefresh);

            system->count++;

            drmModeFreeEncoder(encoder);
            drmModeFreeConnector(conn);
        }

        drmModeFreeResources(resources);

        /*
         * IMPORTANTE:
         *
         * No cerramos fd aquí.
         *
         * Todas las salidas encontradas en esta
         * card utilizan este mismo fd.
         */
    }

    closedir(dir);

    printf("\nSalidas DRM encontradas: %d\n",
           system->count);

    return system->count;
}
int cargarfuente()
{
    if (FT_Init_FreeType(&ft))
    {
        printf("Error FreeType\n");
        return EXIT_FAILURE;
    }

    if (FT_New_Memory_Face(ft, __3270_ttf, __3270_ttf_len, 0, &face))
    {
        printf("No se pudo abrir la fuente\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
int drm_present(DrmOutput *output)
{
    uint32_t connector_id = output->connector_id;

    int ret = drmModeSetCrtc(
        output->fd,
        output->crtc_id,
        output->fb_id,
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

    return 0;
}

int main(int argc, char const *argv[])
{
    if (cargarfuente() != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }
    DrmSystem system = {0};

    int n = drm_find_outputs(&system);

    for (int i = 0; i < system.count; i++)
    {
        DrmOutput *output = &system.outputs[i];

        if (drm_create_framebuffer(output) < 0)
            return EXIT_FAILURE;

        if (drm_create_surface(output) < 0)
            return EXIT_FAILURE;

        if (drm_draw_splash(output, &face) < 0)
            return EXIT_FAILURE;

        if (drm_present(output) < 0)
            return EXIT_FAILURE;
    }
    sleep(10);
    // drmModeConnector *connz = NULL;
    // uint32_t connector_id = 0;
    // int fd = open("/dev/dri/card1", O_RDWR);
    // if (fd < 0)
    // {
    //     perror("open");
    //     return EXIT_FAILURE;
    // }
    // drmModeRes *resources = drmModeGetResources(fd);
    // if (!resources)
    // {
    //     printf("No se pudieron obtener los recursos DRM.\n");

    //     close(fd);

    //     return EXIT_FAILURE;
    // }
    // printf("Framebuffers : %d\n", resources->count_fbs);

    // printf("CRTCs        : %d\n", resources->count_crtcs);

    // printf("Connectors   : %d\n", resources->count_connectors);

    // printf("Encoders     : %d\n", resources->count_encoders);

    // for (size_t i = 0; i < resources->count_connectors; i++)
    // {
    //     uint32_t id = resources->connectors[i];
    //     drmModeConnector *conn = drmModeGetConnector(fd, id);
    //     printf("ID: %u\n", conn->connector_id);
    //     printf("Modos: %d\n", conn->count_modes);
    //     switch (conn->connection)
    //     {
    //     case DRM_MODE_CONNECTED:
    //         printf("Estado: Conectado\n");
    //         connector_id = conn->connector_id;
    //         break;

    //     case DRM_MODE_DISCONNECTED:
    //         printf("Estado: Desconectado\n");
    //         break;

    //     default:
    //         printf("Estado: Desconocido\n");
    //     }
    //     for (size_t i = 0; i < conn->count_modes; i++)
    //     {
    //         /* code */
    //         drmModeModeInfo mode = conn->modes[i];
    //         printf("modo ancho: %u\n", mode.hdisplay);
    //         printf("modo alto: %u\n", mode.vdisplay);
    //         printf("modo refresco: %u\n", mode.vrefresh);
    //         if (mode.hdisplay == 1920 && mode.vdisplay == 1080 &&
    //             mode.vrefresh == 60)
    //         {
    //             modez = mode;
    //             printf("Modo elegido: %ux%u @ %u Hz\n", modez.hdisplay, modez.vdisplay,
    //                    modez.vrefresh);
    //         }
    //     }

    //     // liberar recursos conn
    //     drmModeFreeConnector(conn);
    // }
    // connz = drmModeGetConnector(fd, connector_id);
    // if (!connz)
    // {
    //     perror("drmModeGetConnector");
    //     return EXIT_FAILURE;
    // }
    // struct drm_mode_create_dumb create = {0};
    // int width = 1920;
    // int height = 1080;
    // create.width = 1920;
    // create.height = 1080;
    // create.bpp = 32;
    // if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0)
    // {
    //     /* code */
    //     perror("CREATE_DUMB");
    // }

    // printf("Handle : %u\n", create.handle);

    // printf("Pitch  : %u\n", create.pitch);

    // printf("Size   : %llu\n", (unsigned long long)create.size);

    // uint32_t fb_id = 0;
    // /*  if (drmModeAddFB(fd, create.width, create.height, 24, 32, create.pitch,
    //                   create.handle, &fb_id)) {

    //    perror("drmModeAddFB");
    //  } */
    // uint32_t handles[4] = {0};
    // uint32_t pitches[4] = {0};
    // uint32_t offsets[4] = {0};
    // handles[0] = create.handle;
    // pitches[0] = create.pitch;
    // offsets[0] = 0;

    // printf("Framebuffer ID: %u\n", fb_id);

    // struct drm_mode_map_dumb map = {0};

    // map.handle = create.handle;
    // printf("create.pitch = %u\n", create.pitch);
    // printf("create.handle = %u\n", create.handle);
    // if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0)
    // {
    //     /* code */
    //     perror("MAP_DUMB");
    // }
    // printf("offset = %llu\n", (unsigned long long)map.offset);

    // offsets[0] = 0;
    // int ret_fb =
    //     drmModeAddFB2(fd, create.width, create.height, DRM_FORMAT_XRGB8888,
    //                   handles, pitches, offsets, &fb_id, 0);

    // if (ret_fb != 0)
    // {
    //     perror("drmModeAddFB2");
    //     return EXIT_FAILURE;
    // }

    // // printf("fb_id = %u\n", fb_id);
    // /*
    //   void *pixels =
    //       mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
    //   map.offset);
    //  */
    // uint32_t *pixels = (uint32_t *)mmap(NULL, create.size, PROT_READ | PROT_WRITE,
    //                                     MAP_SHARED, fd, map.offset);

    // if (pixels == MAP_FAILED)
    // {
    //     perror("mmap");
    // }
    // printf("direcion de pixels %p", pixels);
    // // drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

    // drmModeEncoder *encoder = drmModeGetEncoder(fd, connz->encoder_id);

    // if (!encoder)
    // {
    //     perror("drmModeGetEncoder");
    //     return EXIT_FAILURE;
    // }
    // printf("fb_id      = %u\n", fb_id);
    // printf("connector  = %u\n", connector_id);
    // printf("crtc       = %u\n", encoder->crtc_id);
    // printf("mode       = %s\n", modez.name);
    // printf("Encoder ID : %u\n", encoder->encoder_id);
    // printf("possible_crtcs = 0x%x\n", encoder->possible_crtcs);
    // printf("CRTC ID    : %u\n", encoder->crtc_id);
    // drmModeCrtc *old_crtc = drmModeGetCrtc(fd, encoder->crtc_id);
    // uint32_t conn_id = connz->connector_id;
    // uint32_t *fb = pixels;

    // /* for (uint32_t y = 0; y < create.height; y++) {
    //   for (uint32_t x = 0; x < create.width; x++) {
    //     fb[y * (create.pitch / 4) + x] = 0xFFFFFF00; // Azul
    //   }
    // }
    // printf("Primer pixel = %08X\n", pixels[0]); */
    // // char path_txt2[] = "M 10 10 L 90 10 L 90 90 L 10 90 Z";
    // char path1[] =
    //     "m 131.91082,173.66089 "
    //     "h 14.23729 "
    //     "l 0.44492,-59.91526 "
    //     "39.89406,-16.313551 "
    //     "-0.29661,77.118641 "
    //     "6.22881,-0.29661 "
    //     "-0.1483,-84.682206 "
    //     "37.22458,21.800846 "
    //     "0.74152,-17.796607 "
    //     "35.74153,21.504237 "
    //     "V 96.987159 "
    //     "l 35,21.652541 "
    //     "0.29662,-18.24152 "
    //     "12.90254,7.71186 "
    //     "0.59322,-51.165253 "
    //     "13.79236,0.296608 "
    //     "1.03814,60.211865 "
    //     "6.82202,3.55932 "
    //     "0.29663,-63.622878 "
    //     "h 14.23728 "
    //     "l 1.48304,67.775418 "
    //     "18.98306,3.26272 "
    //     "0.14831,45.82627 "
    //     "h 15.57204 "
    //     "l 0.2966,4.15254 "
    //     "-255.52966,-0.29661 "
    //     "z";

    // char path2[] =
    //     "m 154.7498,155.27106 "
    //     "-0.59322,19.27966 "
    //     "14.3856,0.1483 "
    //     "0.59322,-19.57627 "
    //     "z";

    // char path3[] =
    //     "m 203.24557,128.72445 "
    //     "0.1483,-6.82204 "
    //     "156.16527,14.23729 "
    //     "0.1483,4.15254 "
    //     "z";

    // char path4[] =
    //     "m 252.03792,156.01257 "
    //     "h 19.42798 "
    //     "v 7.41525 "
    //     "h -19.42798 "
    //     "z";

    // char path5[] =
    //     "m 279.4744,156.60579 "
    //     "h 16.01694 "
    //     "v 7.11864 "
    //     "H 279.4744 "
    //     "Z";

    // char path6[] =
    //     "m 303.49979,156.9024 "
    //     "h 15.1271 "
    //     "v 6.97035 "
    //     "h -15.1271 "
    //     "z";

    // char path7[] =
    //     "m 324.26251,157.49564 "
    //     "h 15.57204 "
    //     "v 6.22881 "
    //     "h -15.57204 "
    //     "z";

    // char path8[] =
    //     "m 346.5083,157.94055 "
    //     "h 13.34745 "
    //     "v 5.48729 "
    //     "H 346.5083 "
    //     "Z";

    // char path9[] =
    //     "m 225.63963,154.38123 "
    //     "h 20.02119 "
    //     "v 9.04661 "
    //     "h -20.02119 "
    //     "z";

    // surface = cairo_image_surface_create_for_data(
    //     (unsigned char *)pixels,
    //     CAIRO_FORMAT_RGB24,
    //     create.width,
    //     create.height,
    //     create.pitch);

    // cr = cairo_create(surface);

    // /* Fondo negro */
    // cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    // cairo_paint(cr);

    // /* Logo */
    // cairo_save(cr);

    // cairo_translate(cr, 500, 250);
    // cairo_scale(cr, 2.0, 2.0);

    // cairo_set_source_rgb(cr, 0.2, 1.0, 0.2);
    // cairo_set_line_width(cr, 1.0);

    // parse_svg_path(cr, path1);
    // cairo_stroke(cr);
    // parse_svg_path(cr, path2);
    // cairo_stroke(cr);
    // parse_svg_path(cr, path3);
    // cairo_stroke(cr);
    // parse_svg_path(cr, path4);
    // cairo_stroke(cr);
    // parse_svg_path(cr, path5);
    // cairo_stroke(cr);
    // parse_svg_path(cr, path6);
    // cairo_stroke(cr);
    // parse_svg_path(cr, path7);
    // cairo_stroke(cr);
    // parse_svg_path(cr, path8);
    // cairo_stroke(cr);
    // parse_svg_path(cr, path9);

    // cairo_stroke(cr);

    // cairo_restore(cr);

    // /* Texto */
    // cairo_font_face_t *font =
    //     cairo_ft_font_face_create_for_ft_face(face, 0);

    // cairo_set_font_face(cr, font);
    // cairo_set_font_size(cr, 64);

    // cairo_set_source_rgb(cr, 0.2, 1.0, 0.2);

    // cairo_move_to(cr, 850, 660);
    // cairo_show_text(cr, "ZaramagaOS");
    // int ret =
    //     drmModeSetCrtc(fd, encoder->crtc_id, fb_id, 0, 0, &conn_id, 1, &modez);
    // sleep(10);
    // if (ret != 0)
    // {
    //     perror("drmModeSetCrtc");
    // }

    // printf("Mostrando framebuffer...\n");
    // getchar();
    // sleep(10);
    // printf("Connector ID: %u\n", conn_id);
    // printf("Encoder ID: %u\n", encoder->encoder_id);
    // printf("CRTC ID: %u\n", encoder->crtc_id);
    // printf("FB ID: %u\n", fb_id);
    // //  liberar recursos
    // drmModeFreeResources(resources);
    // drmModeFreeCrtc(old_crtc);
    // drmModeFreeConnector(connz);
    // close(fd);

    return 0;
}