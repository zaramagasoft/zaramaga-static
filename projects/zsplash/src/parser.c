#include "parser.h"
#include "lexer.h"

#include <stdio.h>
#include <stdarg.h>

static double current_x = 0;
static double current_y = 0;
void debug_printf(cairo_t *cr, double x, double y, const char *fmt, ...);

void debug_text(cairo_t *cr, double x, double y, const char *msg);
void parse_svg_path(cairo_t *cr, char *path)
{
    cairo_new_path(cr);

    char *p = path;
    Token tok;
    char cmd = 0;

    double current_x = 0.0;
    double current_y = 0.0;

    double start_x = 0.0;
    double start_y = 0.0;

    while ((tok = next_token(&p)).type != TOK_END)
    {
        if (tok.type == TOK_CMD)
        {
            cmd = tok.cmd;

            if (cmd == 'Z' || cmd == 'z')
            {
                cairo_close_path(cr);

                current_x = start_x;
                current_y = start_y;
            }

            continue;
        }

        if (tok.type != TOK_NUM)
            continue;

        switch (cmd)
        {
        case 'M':
        {
            double x = tok.number;

            Token t = next_token(&p);
            if (t.type != TOK_NUM)
                break;

            double y = t.number;

            cairo_move_to(cr, x, y);

            current_x = x;
            current_y = y;
            cmd = 'L';
            start_x = x;
            start_y = y;
            break;
        }

        case 'm':
        {
            double dx = tok.number;

            Token t = next_token(&p);
            if (t.type != TOK_NUM)
                break;

            double dy = t.number;

            current_x += dx;
            current_y += dy;

            cairo_move_to(cr, current_x, current_y);
            cmd = 'l';
            start_x = current_x;
            start_y = current_y;
            break;
        }

        case 'L':
        {
            double x = tok.number;

            Token t = next_token(&p);
            if (t.type != TOK_NUM)
                break;

            double y = t.number;

            cairo_line_to(cr, x, y);

            current_x = x;
            current_y = y;
            break;
        }

        case 'l':
        {
            double dx = tok.number;

            Token t = next_token(&p);
            if (t.type != TOK_NUM)
                break;

            double dy = t.number;

            current_x += dx;
            current_y += dy;

            cairo_line_to(cr, current_x, current_y);
            break;
        }

        case 'H':
        {
            current_x = tok.number;
            cairo_line_to(cr, current_x, current_y);
            break;
        }

        case 'h':
        {
            current_x += tok.number;
            cairo_line_to(cr, current_x, current_y);
            break;
        }

        case 'V':
        {
            current_y = tok.number;
            cairo_line_to(cr, current_x, current_y);
            break;
        }

        case 'v':
        {
            current_y += tok.number;
            cairo_line_to(cr, current_x, current_y);
            break;
        }

        default:
            break;
        }
    }
}