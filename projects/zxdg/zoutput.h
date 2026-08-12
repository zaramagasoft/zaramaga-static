#ifndef ZOUTPUT_H
#define ZOUTPUT_H

#include <stdint.h>
#include <wayland-client.h>

#define Z_MAX_OUTPUTS 16

typedef struct
{
    struct wl_output *wl_output;

    uint32_t registry_name;

    int32_t x;
    int32_t y;

    int32_t width;
    int32_t height;

    int32_t refresh;       /* mHz: 60000 = 60 Hz */
    int32_t scale;

    uint32_t transform;

    uint32_t mode_flags;

    char name[64];
    char description[128];

} ZOutput;

typedef struct
{
    ZOutput outputs[Z_MAX_OUTPUTS];

    int count;

} ZOutputManager;

void zoutput_init(ZOutputManager *manager);

int zoutput_bind(
    ZOutputManager *manager,
    struct wl_registry *registry,
    uint32_t name,
    uint32_t version);

void zoutput_print(
    const ZOutputManager *manager);

#endif