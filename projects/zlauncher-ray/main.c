#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

//----------------------------------------------------------
// Layout
//----------------------------------------------------------

static void ButtonPrueba(void)
{
    // lógica del botón
}

//----------------------------------------------------------
// Main
//----------------------------------------------------------

int main(void)
{
    InitWindow(1280, 720, "ZaramagaOS");

    SetTargetFPS(60);

    // Layout generado por rGuiLayout
    const char *ButtonPruebaText = "texto de boton";

    Rectangle layoutRecs[1] = {
        (Rectangle){ 600, 312, 120, 24 }
    };

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(BLACK);

        // Nuestro contenido
        DrawText("ZaramagaOS", 100, 100, 40, GREEN);

        // Layout raygui
        if (GuiButton(layoutRecs[0], ButtonPruebaText))
            ButtonPrueba();

        EndDrawing();
    }

    CloseWindow();

    return 0;
}