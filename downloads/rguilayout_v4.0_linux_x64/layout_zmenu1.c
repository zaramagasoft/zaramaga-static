/*******************************************************************************************
*
*   LayoutName v1.0.0 - Tool Description
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2022 raylib technologies. All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------
static void #icoMute();
static void ButtonSalr();
static void ButtonPoweroff();
static void Buttowifi();
static void Buttonblue();
static void ButtonReboot();

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    // Initialization
    //---------------------------------------------------------------------------------------
    int screenWidth = 800;
    int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "layout_name");

    // layout_name: controls initialization
    //----------------------------------------------------------------------------------
    bool vExtActive = true;
    bool SpinnerVolEditMode = false;
    int SpinnerVolValue = 0;
    bool DropdownBox008EditMode = false;
    int DropdownBox008Active = 0;
    bool TextBoxResEditMode = false;
    char TextBoxResText[128] = "1920x1080";
    bool Spinner0rilloEditMode = false;
    int Spinner0rilloValue = 0;
    bool CheckBoxGamemodeChecked = false;
    //----------------------------------------------------------------------------------

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Implement required update logic
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR))); 

            // raygui: controls drawing
            //----------------------------------------------------------------------------------
            if (DropdownBox008EditMode) GuiLock();

            if (vExtActive)
            {
                vExtActive = !GuiWindowBox((Rectangle){ 0, 0, 400, 952 }, "zaramagaOS");
                if (GuiSpinner((Rectangle){ 96, 160, 264, 40 }, NULL, &SpinnerVolValue, 0, 100, SpinnerVolEditMode)) SpinnerVolEditMode = !SpinnerVolEditMode;
                GuiCheckBox((Rectangle){ 24, 456, 16, 24 }, "GAME MODE", &CheckBoxGamemodeChecked);
            }
            GuiPanel((Rectangle){ 16, 40, 128, 80 }, NULL);
            GuiPanel((Rectangle){ 168, 40, 216, 80 }, NULL);
            GuiGroupBox((Rectangle){ 16, 144, 368, 128 }, "AUDIO");
            GuiGroupBox((Rectangle){ 16, 312, 368, 120 }, "VIDEO");
            GuiGroupBox((Rectangle){ 16, 864, 368, 72 }, "SISTEMA");
            if (GuiButton((Rectangle){ 32, 160, 56, 40 }, "#122#")) #icoMute(); 
            GuiLabel((Rectangle){ 32, 328, 72, 24 }, "RESOLUCION");
            if (GuiTextBox((Rectangle){ 120, 328, 120, 24 }, TextBoxResText, 128, TextBoxResEditMode)) TextBoxResEditMode = !TextBoxResEditMode;
            if (GuiSpinner((Rectangle){ 96, 368, 264, 40 }, NULL, &Spinner0rilloValue, 0, 100, Spinner0rilloEditMode)) Spinner0rilloEditMode = !Spinner0rilloEditMode;
            GuiLabel((Rectangle){ 32, 368, 56, 40 }, "BRILLO");
            if (GuiButton((Rectangle){ 32, 880, 56, 40 }, "#159#salir")) ButtonSalr(); 
            if (GuiButton((Rectangle){ 96, 880, 80, 40 }, "#181#PowerOff")) ButtonPoweroff(); 
            GuiGroupBox((Rectangle){ 16, 504, 368, 120 }, "NET");
            GuiLabel((Rectangle){ 32, 512, 160, 24 }, "IP LOCAL.");
            if (GuiButton((Rectangle){ 32, 584, 120, 24 }, "#188#WIFI")) Buttowifi(); 
            if (GuiButton((Rectangle){ 32, 584, 120, 24 }, "Bluethoot")) Buttonblue(); 
            if (GuiButton((Rectangle){ 184, 880, 80, 40 }, "#211#Reboott")) ButtonReboot(); 
            if (GuiDropdownBox((Rectangle){ 32, 208, 328, 48 }, "fuenteAudio1;FuenteAudio2", &DropdownBox008Active, DropdownBox008EditMode)) DropdownBox008EditMode = !DropdownBox008EditMode;
            
            GuiUnlock();
            //----------------------------------------------------------------------------------

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Controls Functions Definitions (local)
//------------------------------------------------------------------------------------
static void #icoMute()
{
    // TODO: Implement control logic
}
static void ButtonSalr()
{
    // TODO: Implement control logic
}
static void ButtonPoweroff()
{
    // TODO: Implement control logic
}
static void Buttowifi()
{
    // TODO: Implement control logic
}
static void Buttonblue()
{
    // TODO: Implement control logic
}
static void ButtonReboot()
{
    // TODO: Implement control logic
}

