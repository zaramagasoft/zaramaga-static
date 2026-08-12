#include <stdio.h>
#include "zlayout.h"

int main(void)
{
    ZWidget w = {
        .anchor = Z_ANCHOR_CENTER,
        .x = 0,
        .y = 0,
        .width = 400,
        .height = 100
    };

    ZRect r = zlayout_resolve(&w, 1920, 1080);

    printf(
        "x=%.1f y=%.1f w=%.1f h=%.1f\n",
        r.x,
        r.y,
        r.width,
        r.height
    );

    return 0;
}