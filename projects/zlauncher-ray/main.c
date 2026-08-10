#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define GUI_LAYOUT_HOME_IMPLEMENTATION
#include "gui_layout_home.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>

#define MAX_MICE 16

typedef struct
{
    int fd;
    char path[64];
    char name[256];
} MouseDevice;

static MouseDevice mice[MAX_MICE];
static int mouse_count = 0;


/* ---------------------------------------------------------
 * Bit helper
 * --------------------------------------------------------- */

#define TEST_BIT(bit, array) \
    ((array[(bit) / (sizeof(unsigned long) * 8)] >> \
     ((bit) % (sizeof(unsigned long) * 8))) & 1)


/* ---------------------------------------------------------
 * Comprobar si un eventX es realmente un ratón
 * --------------------------------------------------------- */

static int is_mouse_device(int fd)
{
    unsigned long evbits[
        EV_MAX / (sizeof(unsigned long) * 8) + 1
    ] = {0};

    if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0)
        return 0;

    /* Tiene eventos relativos */
    if (!TEST_BIT(EV_REL, evbits))
        return 0;

    unsigned long relbits[
        REL_MAX / (sizeof(unsigned long) * 8) + 1
    ] = {0};

    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbits)), relbits) < 0)
        return 0;

    /* Necesitamos X e Y */
    if (!TEST_BIT(REL_X, relbits))
        return 0;

    if (!TEST_BIT(REL_Y, relbits))
        return 0;

    return 1;
}


/* ---------------------------------------------------------
 * Detectar TODOS los ratones
 * --------------------------------------------------------- */

static int detect_mice(void)
{
    DIR *dir = opendir("/dev/input");

    if (!dir)
    {
        perror("opendir /dev/input");
        return -1;
    }

    struct dirent *entry;

    mouse_count = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;

        if (mouse_count >= MAX_MICE)
            break;

        char path[128];

        snprintf(
            path,
            sizeof(path),
            "/dev/input/%s",
            entry->d_name
        );

        int fd = open(
            path,
            O_RDONLY | O_NONBLOCK
        );

        if (fd < 0)
            continue;

        if (!is_mouse_device(fd))
        {
            close(fd);
            continue;
        }

        mice[mouse_count].fd = fd;

        strncpy(
            mice[mouse_count].path,
            path,
            sizeof(mice[mouse_count].path) - 1
        );

        mice[mouse_count].path[
            sizeof(mice[mouse_count].path) - 1
        ] = '\0';

        memset(
            mice[mouse_count].name,
            0,
            sizeof(mice[mouse_count].name)
        );

        ioctl(
            fd,
            EVIOCGNAME(sizeof(mice[mouse_count].name)),
            mice[mouse_count].name
        );

        printf(
            "ZINPUT: mouse encontrado: %s (%s)\n",
            mice[mouse_count].path,
            mice[mouse_count].name
        );

        mouse_count++;
    }

    closedir(dir);

    printf(
        "ZINPUT: %d dispositivo(s) de raton\n",
        mouse_count
    );

    return mouse_count;
}


/* ---------------------------------------------------------
 * Leer ratones
 * --------------------------------------------------------- */

static void update_mouse(
    int *mx,
    int *my,
    int *left,
    int *right)
{
    for (int i = 0; i < mouse_count; i++)
    {
        struct input_event ev;

        while (read(
            mice[i].fd,
            &ev,
            sizeof(ev)
        ) == sizeof(ev))
        {
            if (ev.type == EV_REL)
            {
                if (ev.code == REL_X)
                {
                    *mx += ev.value;
                }
                else if (ev.code == REL_Y)
                {
                    *my += ev.value;
                }
            }

            else if (ev.type == EV_KEY)
            {
                if (ev.code == BTN_LEFT)
                {
                    *left = ev.value;
                }
                else if (ev.code == BTN_RIGHT)
                {
                    *right = ev.value;
                }
            }
        }
    }
}


/* ---------------------------------------------------------
 * Limitar cursor a pantalla
 * --------------------------------------------------------- */

static void clamp_mouse(
    int *x,
    int *y,
    int width,
    int height)
{
    if (*x < 0)
        *x = 0;

    if (*y < 0)
        *y = 0;

    if (*x >= width)
        *x = width - 1;

    if (*y >= height)
        *y = height - 1;
}


/* ---------------------------------------------------------
 * Cerrar dispositivos
 * --------------------------------------------------------- */

static void close_mice(void)
{
    for (int i = 0; i < mouse_count; i++)
    {
        if (mice[i].fd >= 0)
            close(mice[i].fd);

        mice[i].fd = -1;
    }

    mouse_count = 0;
}


/* ---------------------------------------------------------
 * MAIN
 * --------------------------------------------------------- */

int main(void)
{
    const int screen_width = 1280;
    const int screen_height = 720;

    int mouse_x = screen_width / 2;
    int mouse_y = screen_height / 2;

    int mouse_left = 0;
    int mouse_right = 0;

    InitWindow(
        screen_width,
        screen_height,
        "ZaramagaOS"
    );

    /*
     * Nosotros controlamos el cursor.
     */
    DisableCursor();

    SetTargetFPS(30);

    /*
     * Detectar dispositivos antes del loop.
     */
    if (detect_mice() <= 0)
    {
        printf(
            "ZINPUT: ERROR: no se encontro ningun raton\n"
        );
    }

    /*
     * Inicializar layout generado por raygui Layout.
     */
    GuiLayoutHomeState home =
        InitGuiLayoutHome();


    /* -----------------------------------------------------
     * LOOP
     * ----------------------------------------------------- */

    while (!WindowShouldClose())
    {
        /*
         * Leer nuestros dispositivos.
         */
        update_mouse(
            &mouse_x,
            &mouse_y,
            &mouse_left,
            &mouse_right
        );

        /*
         * Evitar que el cursor salga de pantalla.
         */
        clamp_mouse(
            &mouse_x,
            &mouse_y,
            screen_width,
            screen_height
        );


        /*
         * Intentamos mantener sincronizado el cursor
         * interno de raylib.
         *
         * En DRM puede no estar implementado por la
         * plataforma, por eso NO dependemos de esto
         * para dibujar nuestro cursor.
         */
        SetMousePosition(
            mouse_x,
            mouse_y
        );


        /* -------------------------------------------------
         * DRAW
         * ------------------------------------------------- */

        BeginDrawing();

        ClearBackground(BLACK);

        /*
         * Layout generado por rGuiLayout.
         */
        GuiLayoutHome(&home);


        /*
         * Nuestro cursor.
         */
        DrawTriangle(
            (Vector2){
                mouse_x,
                mouse_y
            },

            (Vector2){
                mouse_x,
                mouse_y + 18
            },

            (Vector2){
                mouse_x + 12,
                mouse_y + 12
            },

            GREEN
        );


        /*
         * Información de debug.
         */
        DrawText(
            TextFormat(
                "MOUSE %d %d",
                mouse_x,
                mouse_y
            ),
            20,
            20,
            20,
            GREEN
        );


        DrawText(
            TextFormat(
                "MICE: %d",
                mouse_count
            ),
            20,
            45,
            20,
            GREEN
        );


        EndDrawing();
    }


    /* -----------------------------------------------------
     * CLEANUP
     * ----------------------------------------------------- */

    close_mice();

    CloseWindow();

    return 0;
}