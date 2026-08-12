#include "zlayout.h"

ZRect zlayout_resolve(
    const ZWidget *widget,
    int screen_width,
    int screen_height)
{
    ZRect rect;

    rect.width = widget->width;
    rect.height = widget->height;

    switch (widget->anchor)
    {
        case Z_ANCHOR_TOP_LEFT:
            rect.x = widget->x;
            rect.y = widget->y;
            break;

        case Z_ANCHOR_TOP:
            rect.x = (screen_width - widget->width) / 2.0f
                   + widget->x;

            rect.y = widget->y;
            break;

        case Z_ANCHOR_TOP_RIGHT:
            rect.x = screen_width
                   - widget->width
                   - widget->x;

            rect.y = widget->y;
            break;


        case Z_ANCHOR_LEFT:
            rect.x = widget->x;

            rect.y = (screen_height - widget->height) / 2.0f
                   + widget->y;
            break;

        case Z_ANCHOR_CENTER:
            rect.x = (screen_width - widget->width) / 2.0f
                   + widget->x;

            rect.y = (screen_height - widget->height) / 2.0f
                   + widget->y;
            break;

        case Z_ANCHOR_RIGHT:
            rect.x = screen_width
                   - widget->width
                   - widget->x;

            rect.y = (screen_height - widget->height) / 2.0f
                   + widget->y;
            break;


        case Z_ANCHOR_BOTTOM_LEFT:
            rect.x = widget->x;

            rect.y = screen_height
                   - widget->height
                   - widget->y;
            break;

        case Z_ANCHOR_BOTTOM:
            rect.x = (screen_width - widget->width) / 2.0f
                   + widget->x;

            rect.y = screen_height
                   - widget->height
                   - widget->y;
            break;

        case Z_ANCHOR_BOTTOM_RIGHT:
            rect.x = screen_width
                   - widget->width
                   - widget->x;

            rect.y = screen_height
                   - widget->height
                   - widget->y;
            break;
    }

    return rect;
}