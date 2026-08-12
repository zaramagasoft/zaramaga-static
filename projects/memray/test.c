#include "raylib.h"
#include "rlgl.h"
#include <stdio.h>

#define WIDTH 800
#define HEIGHT 600

// Framebuffer permanente en BSS
static unsigned char g_framebuffer[WIDTH * HEIGHT * 4];

// Puntero constante al framebuffer
unsigned char *const g_pBuffer = g_framebuffer;

int main(void)
{
    InitWindow(WIDTH, HEIGHT, "PLATFORM MEMORY - GLOBAL BUFFER");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);
        DrawText("HOLA SOFT", 300, 280, 30, WHITE);
        Vector2 mouse = GetMousePosition();

        printf("Mouse: %.0f %.0f\n", mouse.x, mouse.y);
        if (CheckCollisionPointRec(
                mouse,
                (Rectangle){300, 280, 200, 40}))
        {
            printf("HOVER!\n");
        }
        EndDrawing();

        // RLSW -> nuestro framebuffer global
        rlCopyFramebuffer(
            0,
            0,
            WIDTH,
            HEIGHT,
            RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            g_pBuffer);

        printf(
            "Direccion: %p | "
            "Pixel 0: R=%u G=%u B=%u A=%u\n",
            (void *)g_pBuffer,
            g_pBuffer[0],
            g_pBuffer[1],
            g_pBuffer[2],
            g_pBuffer[3]);
    }

    CloseWindow();

    return 0;
}