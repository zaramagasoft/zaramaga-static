#include "parser.h"

void render_svg(cairo_t *cr, const char *path)
{
    cairo_new_path(cr);

    parse_svg_path(cr, path);

    cairo_fill(cr);
}