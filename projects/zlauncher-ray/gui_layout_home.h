/*******************************************************************************************
*
*   LayoutHome v1.0.0 - Tool Description
*
*   MODULE USAGE:
*       #define GUI_LAYOUT_HOME_IMPLEMENTATION
*       #include "gui_layout_home.h"
*
*       INIT: GuiLayoutHomeState state = InitGuiLayoutHome();
*       DRAW: GuiLayoutHome(&state);
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

// WARNING: raygui implementation is expected to be defined before including this header
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>     // Required for: strcpy()

#ifndef GUI_LAYOUT_HOME_H
#define GUI_LAYOUT_HOME_H

typedef struct {
    // Define anchors
    Vector2 anchor01;            // ANCHOR ID:1
    Vector2 anchor02;            // ANCHOR ID:2
    
    // Define controls variables
    bool CheckBoxExAUTOBOOTChecked;            // CheckBoxEx: CheckBoxExAUTOBOOT

    // Define rectangles
    Rectangle layoutRecs[21];

    // Custom state variables (depend on development software)
    // NOTE: This variables should be added manually if required

} GuiLayoutHomeState;

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// ...

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
GuiLayoutHomeState InitGuiLayoutHome(void);
void GuiLayoutHome(GuiLayoutHomeState *state);
static void ButtonSWAYPRINCIPAL();                // Button: ButtonSWAYPRINCIPAL logic
static void ButtonSWAYGAMER();                // Button: ButtonSWAYGAMER logic
static void ButtonSTEAMD();                // Button: ButtonSTEAMD logic
static void ButtonKONSOLE();                // Button: ButtonKONSOLE logic
static void ButtonWEBGAMESCOPE();                // Button: ButtonWEBGAMESCOPE logic
static void ButtonTTY();                // Button: ButtonTTY logic
static void ButtonOTHER();                // Button: ButtonOTHER logic

#ifdef __cplusplus
}
#endif

#endif // GUI_LAYOUT_HOME_H

/***********************************************************************************
*
*   GUI_LAYOUT_HOME IMPLEMENTATION
*
************************************************************************************/
#if defined(GUI_LAYOUT_HOME_IMPLEMENTATION)

#include "raygui.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Internal Module Functions Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
GuiLayoutHomeState InitGuiLayoutHome(void)
{
    GuiLayoutHomeState state = { 0 };

    // Init anchors
    state.anchor01 = (Vector2){ 72, 144 };            // ANCHOR ID:1
    state.anchor02 = (Vector2){ 792, 144 };            // ANCHOR ID:2
    
    // Initilize controls variables
    state.CheckBoxExAUTOBOOTChecked = false;            // CheckBoxEx: CheckBoxExAUTOBOOT

    // Init controls rectangles
    state.layoutRecs[0] = (Rectangle){ 72, 48, 176, 24 };// Label: LabelKERNEL
    state.layoutRecs[1] = (Rectangle){ 264, 48, 176, 24 };// Label: LabelCPU
    state.layoutRecs[2] = (Rectangle){ 456, 48, 176, 24 };// Label: LabelRAM
    state.layoutRecs[3] = (Rectangle){ 648, 48, 176, 24 };// Label: LabelGPU
    state.layoutRecs[4] = (Rectangle){ 360, 216, 384, 240 };// DummyRec: DummyRecLOGO
    state.layoutRecs[5] = (Rectangle){ 840, 48, 176, 24 };// Label: LabelDATEHOUR
    state.layoutRecs[6] = (Rectangle){ 72, 600, 176, 24 };// Label: LabelAUDIO
    state.layoutRecs[7] = (Rectangle){ 264, 600, 176, 24 };// Label: LabelSCREENS
    state.layoutRecs[8] = (Rectangle){ 456, 600, 176, 24 };// Label: LabelNET
    state.layoutRecs[9] = (Rectangle){ 648, 600, 176, 24 };// Label: LabelUSERS
    state.layoutRecs[10] = (Rectangle){ 840, 600, 176, 24 };// Label: LabelAVANCED
    state.layoutRecs[11] = (Rectangle){ state.anchor01.x + 0, state.anchor01.y + 0, 240, 408 };// GroupBox: GroupBox011
    state.layoutRecs[12] = (Rectangle){ state.anchor01.x + 24, state.anchor01.y + 24, 24, 24 };// CheckBoxEx: CheckBoxExAUTOBOOT
    state.layoutRecs[13] = (Rectangle){ state.anchor01.x + 24, state.anchor01.y + 72, 192, 24 };// Button: ButtonSWAYPRINCIPAL
    state.layoutRecs[14] = (Rectangle){ state.anchor01.x + 24, state.anchor01.y + 120, 192, 24 };// Button: ButtonSWAYGAMER
    state.layoutRecs[15] = (Rectangle){ state.anchor01.x + 24, state.anchor01.y + 168, 192, 24 };// Button: ButtonSTEAMD
    state.layoutRecs[16] = (Rectangle){ state.anchor01.x + 24, state.anchor01.y + 216, 192, 24 };// Button: ButtonKONSOLE
    state.layoutRecs[17] = (Rectangle){ state.anchor01.x + 24, state.anchor01.y + 264, 192, 24 };// Button: ButtonWEBGAMESCOPE
    state.layoutRecs[18] = (Rectangle){ state.anchor01.x + 24, state.anchor01.y + 312, 192, 24 };// Button: ButtonTTY
    state.layoutRecs[19] = (Rectangle){ state.anchor01.x + 24, state.anchor01.y + 360, 192, 24 };// Button: ButtonOTHER
    state.layoutRecs[20] = (Rectangle){ state.anchor02.x + 0, state.anchor02.y + 0, 232, 408 };// GroupBox: GroupBox020

    // Custom variables initialization

    return state;
}
// Button: ButtonSWAYPRINCIPAL logic
static void ButtonSWAYPRINCIPAL()
{
    // TODO: Implement control logic
}
// Button: ButtonSWAYGAMER logic
static void ButtonSWAYGAMER()
{
    // TODO: Implement control logic
}
// Button: ButtonSTEAMD logic
static void ButtonSTEAMD()
{
    // TODO: Implement control logic
}
// Button: ButtonKONSOLE logic
static void ButtonKONSOLE()
{
    // TODO: Implement control logic
}
// Button: ButtonWEBGAMESCOPE logic
static void ButtonWEBGAMESCOPE()
{
    // TODO: Implement control logic
}
// Button: ButtonTTY logic
static void ButtonTTY()
{
    // TODO: Implement control logic
}
// Button: ButtonOTHER logic
static void ButtonOTHER()
{
    // TODO: Implement control logic
}


void GuiLayoutHome(GuiLayoutHomeState *state)
{
    // Const text
    const char *LabelKERNELText = "KERNEL";    // LABEL: LabelKERNEL
    const char *LabelCPUText = "CPU";    // LABEL: LabelCPU
    const char *LabelRAMText = "RAM";    // LABEL: LabelRAM
    const char *LabelGPUText = "GPU";    // LABEL: LabelGPU
    const char *DummyRecLOGOText = "LOGO";    // DUMMYREC: DummyRecLOGO
    const char *LabelDATEHOURText = "FECHA/HORA";    // LABEL: LabelDATEHOUR
    const char *LabelAUDIOText = "AUDIO";    // LABEL: LabelAUDIO
    const char *LabelSCREENSText = "SCREENS";    // LABEL: LabelSCREENS
    const char *LabelNETText = "NET";    // LABEL: LabelNET
    const char *LabelUSERSText = "USERS";    // LABEL: LabelUSERS
    const char *LabelAVANCEDText = "AVANCED";    // LABEL: LabelAVANCED
    const char *GroupBox011Text = "LAUNCHER";    // GROUPBOX: GroupBox011
    const char *CheckBoxExAUTOBOOTText = "AUTO LAUNCH ON BOOT";    // CHECKBOXEX: CheckBoxExAUTOBOOT
    const char *ButtonSWAYPRINCIPALText = "SWAY PRINCIPAL";    // BUTTON: ButtonSWAYPRINCIPAL
    const char *ButtonSWAYGAMERText = "SWAY GAMER CONF";    // BUTTON: ButtonSWAYGAMER
    const char *ButtonSTEAMDText = "STEAMDEACK MODE";    // BUTTON: ButtonSTEAMD
    const char *ButtonKONSOLEText = "KONSOLE/DRM MODE";    // BUTTON: ButtonKONSOLE
    const char *ButtonWEBGAMESCOPEText = "WEB MODE";    // BUTTON: ButtonWEBGAMESCOPE
    const char *ButtonTTYText = "TTY";    // BUTTON: ButtonTTY
    const char *ButtonOTHERText = "OTHERS";    // BUTTON: ButtonOTHER
    const char *GroupBox020Text = "SYSTEM";    // GROUPBOX: GroupBox020
    
    // Draw controls
    GuiLabel(state->layoutRecs[0], LabelKERNELText);
    GuiLabel(state->layoutRecs[1], LabelCPUText);
    GuiLabel(state->layoutRecs[2], LabelRAMText);
    GuiLabel(state->layoutRecs[3], LabelGPUText);
    GuiDummyRec(state->layoutRecs[4], DummyRecLOGOText);
    GuiLabel(state->layoutRecs[5], LabelDATEHOURText);
    GuiLabel(state->layoutRecs[6], LabelAUDIOText);
    GuiLabel(state->layoutRecs[7], LabelSCREENSText);
    GuiLabel(state->layoutRecs[8], LabelNETText);
    GuiLabel(state->layoutRecs[9], LabelUSERSText);
    GuiLabel(state->layoutRecs[10], LabelAVANCEDText);
    GuiGroupBox(state->layoutRecs[11], GroupBox011Text);
    GuiCheckBox(state->layoutRecs[12], CheckBoxExAUTOBOOTText, &state->CheckBoxExAUTOBOOTChecked);
    if (GuiButton(state->layoutRecs[13], ButtonSWAYPRINCIPALText)) ButtonSWAYPRINCIPAL(); 
    if (GuiButton(state->layoutRecs[14], ButtonSWAYGAMERText)) ButtonSWAYGAMER(); 
    if (GuiButton(state->layoutRecs[15], ButtonSTEAMDText)) ButtonSTEAMD(); 
    if (GuiButton(state->layoutRecs[16], ButtonKONSOLEText)) ButtonKONSOLE(); 
    if (GuiButton(state->layoutRecs[17], ButtonWEBGAMESCOPEText)) ButtonWEBGAMESCOPE(); 
    if (GuiButton(state->layoutRecs[18], ButtonTTYText)) ButtonTTY(); 
    if (GuiButton(state->layoutRecs[19], ButtonOTHERText)) ButtonOTHER(); 
    GuiGroupBox(state->layoutRecs[20], GroupBox020Text);
}

#endif // GUI_LAYOUT_HOME_IMPLEMENTATION
