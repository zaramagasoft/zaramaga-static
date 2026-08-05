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
#include "font3270.h"
#include "logo_svg.h"

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

void render_cube(cairo_t *cr, float angle) {
  // 1. Limpiar el lienzo (Fondo oscuro)
  cairo_set_source_rgb(cr, 0.1, 0.1, 0.12);
  cairo_paint(cr);

  // 2. Proyección 3D a 2D
  Vec2 projected[8];
  float rad = angle * (3.14159f / 180.0f);
  float cos_a = cosf(rad), sin_a = sinf(rad);

  for (int i = 0; i < 8; i++) {
    // Rotación en Y y X
    float x = cube_nodes[i].x * cos_a - cube_nodes[i].z * sin_a;
    float z = cube_nodes[i].x * sin_a + cube_nodes[i].z * cos_a;
    float y = cube_nodes[i].y * cos_a - z * sin_a;
    z = cube_nodes[i].y * sin_a + z * cos_a;

    // Perspectiva simple
    float distance = 3.5f;
    float fov = 500.0f;
    projected[i].x = (x * fov) / (z + distance) + (g_width / 2.0f);
    projected[i].y = (y * fov) / (z + distance) + (g_height / 2.0f);
  }

  // 3. Dibujar aristas con Cairo
  cairo_set_line_width(cr, 4.0);
  cairo_set_source_rgb(cr, 0.2, 0.8, 1.0); // Azul neón

  for (int i = 0; i < 12; i++) {
    Vec2 p1 = projected[cube_edges[i][0]];
    Vec2 p2 = projected[cube_edges[i][1]];
    cairo_move_to(cr, p1.x, p1.y);
    cairo_line_to(cr, p2.x, p2.y);
  }
  cairo_stroke(cr);
}

int main(int argc, char const *argv[]) {
  FT_Library ft;
  FT_Face face;

  if (FT_Init_FreeType(&ft)) {
    printf("Error FreeType\n");
    return EXIT_FAILURE;
  }

  if (FT_New_Memory_Face(ft, __3270_ttf, __3270_ttf_len, 0, &face)) {
    printf("No se pudo abrir la fuente\n");
    return EXIT_FAILURE;
  }

  int fd = open("/dev/dri/card1", O_RDWR);
  if (fd < 0) {
    // Probar card0 si card1 no abre
    fd = open("/dev/dri/card0", O_RDWR);
    if (fd < 0) {
      perror("open DRM card");
      return EXIT_FAILURE;
    }
  }

  drmModeRes *resources = drmModeGetResources(fd);
  if (!resources) {
    printf("No se pudieron obtener los recursos DRM.\n");
    close(fd);
    return EXIT_FAILURE;
  }

  uint32_t connector_id = 0;

  for (size_t i = 0; i < resources->count_connectors; i++) {
    uint32_t id = resources->connectors[i];
    drmModeConnector *conn = drmModeGetConnector(fd, id);
    
    if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
      connector_id = conn->connector_id;
      
      // Buscar modo 1080p60 o usar el primero que esté disponible
      for (size_t j = 0; j < conn->count_modes; j++) {
        drmModeModeInfo mode = conn->modes[j];
        if (mode.hdisplay == 1920 && mode.vdisplay == 1080 && mode.vrefresh == 60) {
          modez = mode;
          break;
        }
      }
      if (modez.hdisplay == 0) modez = conn->modes[0]; // fallback
      drmModeFreeConnector(conn);
      break;
    }
    drmModeFreeConnector(conn);
  }

  drmModeConnector *connz = drmModeGetConnector(fd, connector_id);
  if (!connz) {
    perror("drmModeGetConnector");
    return EXIT_FAILURE;
  }

  struct drm_mode_create_dumb create = {0};
  create.width = 1920;
  create.height = 1080;
  create.bpp = 32;

  if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
    perror("CREATE_DUMB");
    return EXIT_FAILURE;
  }

  uint32_t fb_id = 0;
  uint32_t handles[4] = {create.handle, 0, 0, 0};
  uint32_t pitches[4] = {create.pitch, 0, 0, 0};
  uint32_t offsets[4] = {0, 0, 0, 0};

  struct drm_mode_map_dumb map = {0};
  map.handle = create.handle;

  if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
    perror("MAP_DUMB");
    return EXIT_FAILURE;
  }

  int ret_fb = drmModeAddFB2(fd, create.width, create.height, DRM_FORMAT_XRGB8888,
                             handles, pitches, offsets, &fb_id, 0);
  if (ret_fb != 0) {
    perror("drmModeAddFB2");
    return EXIT_FAILURE;
  }

  uint32_t *pixels = (uint32_t *)mmap(NULL, create.size, PROT_READ | PROT_WRITE,
                                      MAP_SHARED, fd, map.offset);
  if (pixels == MAP_FAILED) {
    perror("mmap");
    return EXIT_FAILURE;
  }

  drmModeEncoder *encoder = drmModeGetEncoder(fd, connz->encoder_id);
  if (!encoder) {
    perror("drmModeGetEncoder");
    return EXIT_FAILURE;
  }

  drmModeCrtc *old_crtc = drmModeGetCrtc(fd, encoder->crtc_id);
  uint32_t conn_id = connz->connector_id;

  // Asignar el CRTC al monitor
  int ret = drmModeSetCrtc(fd, encoder->crtc_id, fb_id, 0, 0, &conn_id, 1, &modez);
  if (ret != 0) {
    perror("drmModeSetCrtc");
  }

  // --- RENDERIZADO DE CAIRO ---
  cairo_surface_t *surface = cairo_image_surface_create_for_data(
      (unsigned char *)pixels, CAIRO_FORMAT_RGB24, create.width, create.height,
      create.pitch);
  cairo_t *cr = cairo_create(surface);

  cairo_font_face_t *font_3270 = cairo_ft_font_face_create_for_ft_face(face, 0);

  // Bucle de animación del cubo 3D
  float angle = 0.0f;
  for (int frame = 0; frame < 200; frame++) {
    // 1. Dibujar el cubo 3D
    render_cube(cr, angle);

    // 2. Dibujar texto encima con la fuente 3270
    cairo_set_font_face(cr, font_3270);
    cairo_set_font_size(cr, 54);
    cairo_set_source_rgb(cr, 0.0, 1.0, 0.4); // Verde terminal
    cairo_move_to(cr, 100, 150);
    cairo_show_text(cr, "ZaramagaOS - DRM/KMS Native");

    cairo_set_font_size(cr, 32);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_move_to(cr, 100, 220);
    cairo_show_text(cr, "Powered by Cairo + IBM 3270 Font");

    // Sincronizar surface
    cairo_surface_flush(surface);

    angle += 2.0f;
    usleep(16000); // ~60 FPS
  }

  // Limpieza de recursos
  cairo_font_face_destroy(font_3270);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);

  // Restaurar el CRTC antiguo si existía
  if (old_crtc) {
    drmModeSetCrtc(fd, old_crtc->crtc_id, old_crtc->buffer_id, old_crtc->x,
                   old_crtc->y, &conn_id, 1, &old_crtc->mode);
    drmModeFreeCrtc(old_crtc);
  }

  munmap(pixels, create.size);
  drmModeFreeEncoder(encoder);
  drmModeFreeConnector(connz);
  drmModeFreeResources(resources);
  FT_Done_Face(face);
  FT_Done_FreeType(ft);
  close(fd);

  return 0;
}