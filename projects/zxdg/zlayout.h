#ifndef ZLAYOUT_H
#define ZLAYOUT_H

typedef enum
{
    Z_ANCHOR_TOP_LEFT,
    Z_ANCHOR_TOP,
    Z_ANCHOR_TOP_RIGHT,

    Z_ANCHOR_LEFT,
    Z_ANCHOR_CENTER,
    Z_ANCHOR_RIGHT,

    Z_ANCHOR_BOTTOM_LEFT,
    Z_ANCHOR_BOTTOM,
    Z_ANCHOR_BOTTOM_RIGHT

} ZAnchor;

typedef struct
{
    float x;
    float y;
    float width;
    float height;

} ZRect;

typedef struct
{
    ZAnchor anchor;

    float x;
    float y;

    float width;
    float height;

} ZWidget;

ZRect zlayout_resolve(
    const ZWidget *widget,
    int screen_width,
    int screen_height);

#endif