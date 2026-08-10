#include "raylib.h"

int main(void)
{
    InitWindow(1280, 720, "ZaramagaOS");

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);
        DrawText("ZaramagaOS", 100, 100, 40, GREEN);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}