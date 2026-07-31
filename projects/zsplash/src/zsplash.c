#include <fcntl.h>
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

//
// drmModeConnector *connz;

drmModeModeInfo modez;

drmModeCrtc *crtcz;

int main(int argc, char const *argv[]) {
  drmModeConnector *connz = NULL;
  uint32_t connector_id =0;
  int fd = open("/dev/dri/card1", O_RDWR);
  if (fd < 0) {
    perror("open");
    return EXIT_FAILURE;
  }
  drmModeRes *resources = drmModeGetResources(fd);
  if (!resources) {
    printf("No se pudieron obtener los recursos DRM.\n");

    close(fd);

    return EXIT_FAILURE;
  }
  printf("Framebuffers : %d\n", resources->count_fbs);

  printf("CRTCs        : %d\n", resources->count_crtcs);

  printf("Connectors   : %d\n", resources->count_connectors);

  printf("Encoders     : %d\n", resources->count_encoders);

  for (size_t i = 0; i < resources->count_connectors; i++) {
    uint32_t id = resources->connectors[i];
    drmModeConnector *conn = drmModeGetConnector(fd, id);
    printf("ID: %u\n", conn->connector_id);
    printf("Modos: %d\n", conn->count_modes);
    switch (conn->connection) {
    case DRM_MODE_CONNECTED:
      printf("Estado: Conectado\n");
      connector_id = conn->connector_id;
      break;

    case DRM_MODE_DISCONNECTED:
      printf("Estado: Desconectado\n");
      break;

    default:
      printf("Estado: Desconocido\n");
    }
    for (size_t i = 0; i < conn->count_modes; i++) {
      /* code */
      drmModeModeInfo mode = conn->modes[i];
      printf("modo ancho: %u\n", mode.hdisplay);
      printf("modo alto: %u\n", mode.vdisplay);
      printf("modo refresco: %u\n", mode.vrefresh);
      if (mode.hdisplay == 1920 && mode.vdisplay == 1080 &&
          mode.vrefresh == 60) {
        modez = mode;
        printf("Modo elegido: %ux%u @ %u Hz\n", modez.hdisplay, modez.vdisplay,
               modez.vrefresh);
      }
    }

    // liberar recursos conn
    drmModeFreeConnector(conn);
  }
  connz = drmModeGetConnector(fd, connector_id);
  if (!connz) {
    perror("drmModeGetConnector");
    return EXIT_FAILURE;
  }
  struct drm_mode_create_dumb create = {0};
  int width = 1920;
  int height = 1080;
  create.width = 1920;
  create.height = 1080;
  create.bpp = 32;
  if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0) {
    /* code */
    perror("CREATE_DUMB");
  }

  printf("Handle : %u\n", create.handle);

  printf("Pitch  : %u\n", create.pitch);

  printf("Size   : %llu\n", (unsigned long long)create.size);

  uint32_t fb_id = 0;
 /*  if (drmModeAddFB(fd, create.width, create.height, 24, 32, create.pitch,
                   create.handle, &fb_id)) {
     
    perror("drmModeAddFB");
  } */
  uint32_t handles[4] = {0};
  uint32_t pitches[4] = {0};
  uint32_t offsets[4] = {0};
  handles[0] = create.handle;
  pitches[0] = create.pitch;
  offsets[0] = 0;

  printf("Framebuffer ID: %u\n", fb_id);

  struct drm_mode_map_dumb map = {0};

  map.handle = create.handle;
  printf("create.pitch = %u\n", create.pitch);
  printf("create.handle = %u\n", create.handle);
  if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0) {
    /* code */
    perror("MAP_DUMB");
  }
  printf("offset = %llu\n", (unsigned long long)map.offset);

  offsets[0] = 0;
  int ret_fb =
      drmModeAddFB2(fd, create.width, create.height, DRM_FORMAT_XRGB8888,
                    handles, pitches, offsets, &fb_id, 0);

  if (ret_fb != 0) {
    perror("drmModeAddFB2");
    return EXIT_FAILURE;
  }

  //printf("fb_id = %u\n", fb_id);
  /*
    void *pixels =
        mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
    map.offset);
   */
  uint32_t *pixels = (uint32_t *)mmap(NULL, create.size, PROT_READ | PROT_WRITE,
                                      MAP_SHARED, fd, map.offset);

  if (pixels == MAP_FAILED) {
    perror("mmap");
  }
  printf("direcion de pixels %p", pixels);
  // drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

  
  drmModeEncoder *encoder = drmModeGetEncoder(fd, connz->encoder_id);

  if (!encoder) {
    perror("drmModeGetEncoder");
    return EXIT_FAILURE;
  }
  printf("fb_id      = %u\n", fb_id);
  printf("connector  = %u\n", connector_id);
  printf("crtc       = %u\n", encoder->crtc_id);
  printf("mode       = %s\n", modez.name);
  printf("Encoder ID : %u\n", encoder->encoder_id);
  printf("possible_crtcs = 0x%x\n", encoder->possible_crtcs);
  printf("CRTC ID    : %u\n", encoder->crtc_id);
  drmModeCrtc *old_crtc = drmModeGetCrtc(fd, encoder->crtc_id);
  uint32_t conn_id = connz->connector_id;
  uint32_t *fb = pixels;

  /* for (uint32_t y = 0; y < create.height; y++) {
    for (uint32_t x = 0; x < create.width; x++) {
      fb[y * (create.pitch / 4) + x] = 0xFFFFFF00; // Azul
    }
  }
  printf("Primer pixel = %08X\n", pixels[0]); */
  cairo_surface_t *surface = cairo_image_surface_create_for_data(
      (unsigned char *)pixels, CAIRO_FORMAT_RGB24, create.width, create.height,
      create.pitch);

  cairo_t *cr = cairo_create(surface);

  /* Fondo negro */
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_paint(cr);

  /* Rectángulo verde */
  cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
  cairo_rectangle(cr, 200, 200, 500, 250);
  cairo_fill(cr);

  /* Texto */
  cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(cr, 48);

  cairo_move_to(cr, 230, 320);
  cairo_show_text(cr, "ZaramagaOS");

  /* Asegura que Cairo vacía todo al buffer */
  cairo_surface_flush(surface);

  cairo_destroy(cr);
  cairo_surface_destroy(surface);

  int ret =
      drmModeSetCrtc(fd, encoder->crtc_id, fb_id, 0, 0, &conn_id, 1, &modez);
  sleep(10);
  if (ret != 0) {
    perror("drmModeSetCrtc");
  }

  printf("Mostrando framebuffer...\n");
  getchar();
  sleep(10);
  printf("Connector ID: %u\n", conn_id);
  printf("Encoder ID: %u\n", encoder->encoder_id);
  printf("CRTC ID: %u\n", encoder->crtc_id);
  printf("FB ID: %u\n", fb_id);
  //  liberar recursos
  drmModeFreeResources(resources);
  drmModeFreeCrtc(old_crtc);
  drmModeFreeConnector(connz);
  close(fd);

  return 0;
}
