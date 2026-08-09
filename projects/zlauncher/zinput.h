#ifndef ZINPUT_H
#define ZINPUT_H

#include <stdint.h>
#include <sys/time.h>

struct input_event
{
    struct timeval time;
    uint16_t type;
    uint16_t code;
    int32_t value;
};


/* Tipos de evento */

#define EV_SYN      0x00
#define EV_KEY      0x01
#define EV_REL      0x02


/* Movimiento relativo */

#define REL_X       0x00
#define REL_Y       0x01


/* Botones del ratón */

#define BTN_LEFT    0x110
#define BTN_RIGHT   0x111
#define BTN_MIDDLE  0x112


#endif