#include <ft2build.h>
#include <pixman.h>
#include <stdio.h>
#include FT_FREETYPE_H
#include <cairo/cairo.h>
#include <png.h>
#include <stdio.h>
#include <xf86drm.h>

int main(void) {
  printf("SDK Zaramaga Static OK\n");

  printf("drmAvailable() = %d\n", drmAvailable());

  pixman_image_t *img =
      pixman_image_create_bits(PIXMAN_x8r8g8b8, 64, 64, NULL, 0);

  FT_Library ft;
  FT_Init_FreeType(&ft);

  printf("Pixman OK\n");
  printf("FreeType OK\n");

  printf("libpng %s\n", PNG_LIBPNG_VER_STRING);

  FT_Done_FreeType(ft);
  pixman_image_unref(img);
  printf("Cairo %s\n", cairo_version_string());

  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 100, 100);

  cairo_t *cr = cairo_create(surface);

  cairo_set_source_rgb(cr, 0, 1, 0);
  cairo_paint(cr);

  cairo_destroy(cr);
  cairo_surface_destroy(surface);

  puts("Cairo OK");
  return 0;

  return 0;
}