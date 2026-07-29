#include <ft2build.h>
#include <pixman.h>
#include <xf86drm.h>
#include FT_FREETYPE_H

int main(void) {
  FT_Library ft;
  FT_Init_FreeType(&ft);

  pixman_image_t *img =
      pixman_image_create_bits(PIXMAN_a8r8g8b8, 64, 64, NULL, 64 * 4);

  pixman_image_unref(img);
  FT_Done_FreeType(ft);

  return 0;
}