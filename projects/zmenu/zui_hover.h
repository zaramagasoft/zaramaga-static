#ifndef ZUI_HOVER_H
#define ZUI_HOVER_H

#include "nuklear.h"

typedef struct
{
    struct nk_rect volume;
    struct nk_rect bright;
    struct nk_rect contrast;
    struct nk_rect gamma;
    struct nk_rect ping;
    struct nk_rect reboot;
    struct nk_rect exit;
    struct nk_rect power;
    struct nk_rect updown;
    bool is_hovering_ping;
    bool is_hovering_volume;
    bool is_hovering_bright;
    bool is_hovering_contrast;
    bool is_hovering_gamma;
    bool is_hovering_updown;
    bool r_rendered; 
} ZuiHoverRects;

extern ZuiHoverRects g_hover;

#endif // ZUI_HOVER_H