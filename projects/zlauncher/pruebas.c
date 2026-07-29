#include <ft2build.h>
#include <pixman.h>
#include <stdio.h>
#include FT_FREETYPE_H
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

  FT_Done_FreeType(ft);
  pixman_image_unref(img);

  return 0;
}