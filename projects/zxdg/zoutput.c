#include "zoutput.h"

#include <stdio.h>
#include <string.h>


/* ============================================================
 * CALLBACKS
 * ============================================================ */

static void zoutput_geometry(
    void *data,
    struct wl_output *output,
    int32_t x,
    int32_t y,
    int32_t physical_width,
    int32_t physical_height,
    int32_t subpixel,
    const char *make,
    const char *model,
    int32_t transform)
{
    ZOutput *zoutput = data;

    zoutput->x = x;
    zoutput->y = y;
    zoutput->transform = transform;

    (void)output;
    (void)physical_width;
    (void)physical_height;
    (void)subpixel;
    (void)make;
    (void)model;
}


static void zoutput_mode(
    void *data,
    struct wl_output *output,
    uint32_t flags,
    int32_t width,
    int32_t height,
    int32_t refresh)
{
    ZOutput *zoutput = data;

    if (flags & WL_OUTPUT_MODE_CURRENT)
    {
        zoutput->width = width;
        zoutput->height = height;
        zoutput->refresh = refresh;
        zoutput->mode_flags = flags;
    }

    (void)output;
}


static void zoutput_done(
    void *data,
    struct wl_output *output)
{
    /*
     * Wayland indica que ha terminado
     * de enviar el estado inicial del output.
     */

    (void)data;
    (void)output;
}


static void zoutput_scale(
    void *data,
    struct wl_output *output,
    int32_t factor)
{
    ZOutput *zoutput = data;

    zoutput->scale = factor;

    (void)output;
}


static void zoutput_name(
    void *data,
    struct wl_output *output,
    const char *name)
{
    ZOutput *zoutput = data;

    snprintf(
        zoutput->name,
        sizeof(zoutput->name),
        "%s",
        name);

    (void)output;
}


static void zoutput_description(
    void *data,
    struct wl_output *output,
    const char *description)
{
    ZOutput *zoutput = data;

    snprintf(
        zoutput->description,
        sizeof(zoutput->description),
        "%s",
        description);

    (void)output;
}


/* ============================================================
 * LISTENER
 * ============================================================ */

static const struct wl_output_listener zoutput_listener =
{
    .geometry = zoutput_geometry,
    .mode = zoutput_mode,
    .done = zoutput_done,
    .scale = zoutput_scale,
    .name = zoutput_name,
    .description = zoutput_description
};


/* ============================================================
 * INIT
 * ============================================================ */

void zoutput_init(
    ZOutputManager *manager)
{
    memset(
        manager,
        0,
        sizeof(*manager));
}


/* ============================================================
 * BIND
 * ============================================================ */

int zoutput_bind(
    ZOutputManager *manager,
    struct wl_registry *registry,
    uint32_t name,
    uint32_t version)
{
    if (manager->count >= Z_MAX_OUTPUTS)
    {
        fprintf(
            stderr,
            "ZOUTPUT: limite alcanzado\n");

        return -1;
    }

    ZOutput *zoutput =
        &manager->outputs[manager->count];

    memset(
        zoutput,
        0,
        sizeof(*zoutput));

    /*
     * wl_output version 4 nos da:
     *
     * geometry
     * mode
     * done
     * scale
     * name
     * description
     */

    uint32_t bind_version =
        version < 4 ? version : 4;

    zoutput->registry_name = name;

    zoutput->scale = 1;

    zoutput->wl_output =
        wl_registry_bind(
            registry,
            name,
            &wl_output_interface,
            bind_version);

    if (!zoutput->wl_output)
    {
        fprintf(
            stderr,
            "ZOUTPUT: no se pudo bindear output\n");

        return -1;
    }

    wl_output_add_listener(
        zoutput->wl_output,
        &zoutput_listener,
        zoutput);

    manager->count++;

    return 0;
}


/* ============================================================
 * PRINT
 * ============================================================ */

void zoutput_print(
    const ZOutputManager *manager)
{
    printf(
        "\n========================================\n");

    printf(
        "ZOUTPUT: %d monitor(es)\n",
        manager->count);

    printf(
        "========================================\n");

    for (int i = 0;
         i < manager->count;
         i++)
    {
        const ZOutput *o =
            &manager->outputs[i];

        printf(
            "\nOUTPUT %d\n",
            i);

        printf(
            "  Name:        %s\n",
            o->name[0] ? o->name : "(sin nombre)");

        printf(
            "  Description: %s\n",
            o->description[0]
                ? o->description
                : "(sin descripcion)");

        printf(
            "  Position:    %d, %d\n",
            o->x,
            o->y);

        printf(
            "  Resolution:  %d x %d\n",
            o->width,
            o->height);

        printf(
            "  Refresh:     %.3f Hz\n",
            o->refresh / 1000.0);

        printf(
            "  Scale:       %d\n",
            o->scale);

        printf(
            "  Transform:   %u\n",
            o->transform);
    }

    printf(
        "\n========================================\n");

    fflush(stdout);
}