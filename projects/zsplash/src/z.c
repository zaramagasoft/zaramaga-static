#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

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
    drmModeCrtc *old_crtc;

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
void sleep_garantizado(double segundos);
int drm_restore_crtc(DrmOutput *output);
int drm_cleanup(DrmOutput *output);
int drm_find_outputs(DrmSystem *system);
int drm_create_framebuffer(DrmOutput *output);
int drm_present(DrmOutput *output);
int drm_create_surface(DrmOutput *output);
int drm_draw_splash(DrmOutput *output, FT_Face *face, double angle);
// int draw_logo(cairo_t *cr);
int draw_logo(cairo_t *cr);
int draw_logo_centered(cairo_t *cr, int width, int height, double angle);
int draw_title_centered(cairo_t *cr, int width, int height);
void sleep_garantizado(double segundos)
{
    struct timespec req, rem;
    req.tv_sec = (time_t)segundos;
    req.tv_nsec = (long)((segundos - req.tv_sec) * 1e9);

    // nanosleep devuelve -1 si es interrumpido por una señal.
    // El tiempo restante se guarda en 'rem', por lo que seguimos durmiendo
    // hasta que el tiempo total se haya cumplido realmente.
    while (nanosleep(&req, &rem) == -1 && errno == EINTR)
    {
        req = rem;
    }
}
int drm_cleanup(DrmOutput *output)
{
    /* Cairo */
    if (output->cr)
    {
        cairo_destroy(output->cr);
        output->cr = NULL;
    }

    if (output->surface)
    {
        cairo_surface_destroy(output->surface);
        output->surface = NULL;
    }

    /* Framebuffer DRM */
    if (output->fb_id)
    {
        drmModeRmFB(output->fd, output->fb_id);
        output->fb_id = 0;
    }

    /* Desmapear dumb buffer */
    if (output->pixels)
    {
        munmap(output->pixels,
               output->create.size);

        output->pixels = NULL;
    }

    /* Destruir dumb buffer */
    if (output->create.handle)
    {
        struct drm_mode_destroy_dumb destroy = {0};

        destroy.handle = output->create.handle;

        if (drmIoctl(output->fd,
                     DRM_IOCTL_MODE_DESTROY_DUMB,
                     &destroy) < 0)
        {
            perror("DRM_IOCTL_MODE_DESTROY_DUMB");
        }

        output->create.handle = 0;
    }

    /* Liberar CRTC guardado */
    if (output->old_crtc)
    {
        drmModeFreeCrtc(output->old_crtc);
        output->old_crtc = NULL;
    }

    /* Cerrar DRM */
    if (output->fd >= 0)
    {
        close(output->fd);
        output->fd = -1;
    }

    return 0;
}
int drm_restore_crtc(DrmOutput *output)
{
    if (!output->old_crtc)
        return -1;

    int ret = drmModeSetCrtc(
        output->fd,
        output->old_crtc->crtc_id,
        output->old_crtc->buffer_id,
        output->old_crtc->x,
        output->old_crtc->y,
        &output->connector_id,
        1,
        &output->old_crtc->mode);

    if (ret != 0)
    {
        perror("drmModeSetCrtc restore");
        return -1;
    }

    return 0;
}

int draw_title_centered(cairo_t *cr, int width, int height)
{
    const char *text = "ZaramagaOS";

    double font_size = height * 0.055;

    cairo_save(cr);

    cairo_set_font_face(
        cr,
        cairo_ft_font_face_create_for_ft_face(face, 0));

    cairo_set_font_size(cr, font_size);

    cairo_text_extents_t extents;

    cairo_text_extents(
        cr,
        text,
        &extents);

    double x =
        (width - extents.width) / 2.0 - extents.x_bearing;

    /*
     * Ahora queda inmediatamente debajo
     * del logo.
     */
    double y = height * 0.54;

    cairo_move_to(cr, x, y);

    cairo_show_text(cr, text);

    cairo_restore(cr);

    return 0;
}

int draw_logo_centered(cairo_t *cr, int width, int height, double angle)
{
    const double logo_x = 131.9;
    const double logo_y = 51.9;
    const double logo_w = 228.0;
    const double logo_h = 122.0;

    /* Escala proporcional a la resolución */
    double logo_scale = (height / 1080.0) * 1.6;

    double w = logo_w * logo_scale;
    double h = logo_h * logo_scale;

    /* Posición de la esquina superior izquierda del logo */
    double x = (width - w) / 2.0;
    double y = (height - h) / 2.0 - height * 0.12;

    cairo_save(cr);

    /* 
     * TRUCO DE ROTACIÓN:
     * 1. Trasladamos el origen de Cairo al CENTRO del logo.
     */
    cairo_translate(cr, x + (w / 2.0), y + (h / 2.0));

    /* 2. Rotamos la matriz de dibujo el ángulo recibido */
    cairo_rotate(cr, angle);

    /* 3. Escalamos */
    cairo_scale(cr, logo_scale, logo_scale);

    /* 
     * 4. Llevamos el centro geométrico del SVG al origen actual.
     *    El centro del bounding box del SVG es (logo_x + logo_w/2, logo_y + logo_h/2)
     */
    cairo_translate(cr, -(logo_x + logo_w / 2.0), -(logo_y + logo_h / 2.0));

    cairo_set_line_width(cr, 1.0 / logo_scale);

    draw_logo(cr);

    cairo_restore(cr);

    return 0;
}

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
        path9};

    int num_paths = sizeof(paths) / sizeof(paths[0]);

    for (int i = 0; i < num_paths; i++)
    {
        parse_svg_path(cr, paths[i]);
        cairo_stroke(cr);
    }

    return 0;
}
int drm_draw_splash(DrmOutput *output, FT_Face *face, double angle)
{
    cairo_t *cr = output->cr;

    int width = output->create.width;
    int height = output->create.height;

    /* Fondo */
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    /* Verde Zaramaga */
    cairo_set_source_rgb(cr, 0.2, 1.0, 0.2);

    /* Logo con rotación */
    draw_logo_centered(cr, width, height, angle);

    /* Texto fijo */
    draw_title_centered(cr, width, height);

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

            out->fd = dup(fd);

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
            out->old_crtc =
                drmModeGetCrtc(fd, out->crtc_id);

            if (!out->old_crtc)
            {
                printf("    No se pudo guardar el CRTC original\n");

                drmModeFreeEncoder(encoder);
                drmModeFreeConnector(conn);
                continue;
            }

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
        close(fd);
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

    /* Crear los Framebuffers y Superficies Cairo en cada salida */
    for (int i = 0; i < system.count; i++)
    {
        DrmOutput *output = &system.outputs[i];
        if (drm_create_framebuffer(output) < 0) return EXIT_FAILURE;
        if (drm_create_surface(output) < 0) return EXIT_FAILURE;
    }

    /* 
     * BUCLE DE ANIMACIÓN
     * Hacemos p. ej. 300 frames (~5 segundos a 60 FPS)
     */
    double angle = 0.0;

    for (int frame = 0; frame < 300; frame++)
    {
        for (int i = 0; i < system.count; i++)
        {
            DrmOutput *output = &system.outputs[i];

            /* Dibujar splash pasando el ángulo actual */
            drm_draw_splash(output, &face, angle);

            /* Presentar en pantalla */
            drm_present(output);
        }

        /* Incrementar el ángulo en cada paso (~0.05 rad) */
        angle += 0.05;

        /* Tasa de refresco constante (aprox ~60 FPS) */
        sleep_garantizado(0.016);
    }

    /* Restaurar el estado original y liberar recursos */
    for (int i = 0; i < system.count; i++)
    {
        DrmOutput *output = &system.outputs[i];

        if (drm_restore_crtc(output) < 0)
        {
            fprintf(stderr, "Error restaurando CRTC de salida %d\n", i);
        }

        drm_cleanup(output);
    }

    return 0;
}