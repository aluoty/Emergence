#include <raylib.h>
#include "game.h"
#include "ui.h"

int main() {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 800, "Emergence -- Civilization Simulator");
    SetTargetFPS(20);

    uiInit();
    initGame();

    while (!WindowShouldClose()) {
        int w=GetScreenWidth(), h=GetScreenHeight();

        if (IsKeyPressed(KEY_N)) initGame();

        if (!gameOver && !won) {
            if (IsKeyPressed(KEY_Q)) tab=0;
            else if (IsKeyPressed(KEY_W)) tab=1;
            else if (IsKeyPressed(KEY_E)) tab=2;
            else if (IsKeyPressed(KEY_R)) tab=3;
            else if (IsKeyPressed(KEY_A)) tab=4;

            else if (IsKeyPressed(KEY_H)) actHunt();
            else if (IsKeyPressed(KEY_F)) actFarm();
            else if (IsKeyPressed(KEY_G)) actGift();
            else if (IsKeyPressed(KEY_B)) actBuildHomes();
            else if (IsKeyPressed(KEY_U)) actResearch();
            else if (IsKeyPressed(KEY_T)) {
                if (tab==3 && selectedCivIdx!=currentCiv) civTrade(selectedCivIdx);
                else actTrade();
            }
            else if (IsKeyPressed(KEY_Z)) actRest();
            else if (IsKeyPressed(KEY_C)) actHeal();
            else if (IsKeyPressed(KEY_M)) actRecruit();
            else if (IsKeyPressed(KEY_S)) actScout();
            else if (IsKeyPressed(KEY_P)) actTrain();
            else if (IsKeyPressed(KEY_I)) actColonize();
            else if (IsKeyPressed(KEY_Y)) actBuildShip();
            else if (IsKeyPressed(KEY_O)) actExploreGalaxy();
            else if (IsKeyPressed(KEY_K)) saveGame();
            else if (IsKeyPressed(KEY_L)) { if(loadGame())addLog("Game loaded.",(Color){200,200,200,255}); else addLog("No save file.",(Color){200,200,200,255}); }
            else if (IsKeyPressed(KEY_J)) cycleCrop(1);
            else if (IsKeyPressed(KEY_D)) cycleHuntZone(1);

            else if (IsKeyPressed(KEY_ONE)) assignJob(0);
            else if (IsKeyPressed(KEY_TWO)) assignJob(1);
            else if (IsKeyPressed(KEY_THREE)) assignJob(2);
            else if (IsKeyPressed(KEY_FOUR)) assignJob(3);
            else if (IsKeyPressed(KEY_FIVE)) assignJob(4);
            else if (IsKeyPressed(KEY_SIX)) assignJob(5);

            // Civs tab interactions
            if (tab==3) {
                for (int k=KEY_ZERO;k<=KEY_NINE;k++)
                    if (IsKeyPressed(k)) {
                        int idx=k-KEY_ZERO;
                        if (idx>=0 && idx<MAX_CIVS && civs[idx].alive && civs[idx].discovered) {
                            selectedCivIdx = idx;
                            addLogF("Selected: %s",(Color){200,200,255,255},civs[idx].name);
                        }
                    }
                if (IsKeyPressed(KEY_W) && selectedCivIdx!=currentCiv) civWar(selectedCivIdx);
                if (IsKeyPressed(KEY_V) && selectedCivIdx!=currentCiv) switchCiv(selectedCivIdx);
                if (selectedCivIdx==currentCiv && selectedCivIdx>0) selectedCivIdx=0;
            }

            // Context number keys: Buildings tab & Tech tab
            for (int k=KEY_ZERO;k<=KEY_NINE;k++)
                if (IsKeyPressed(k)) {
                    int idx=k-KEY_ZERO;
                    if (tab==1 && idx<MAX_BUILD) buildBuilding(idx);
                    else if (tab==2 && idx<MAX_TECH) learnTech(idx);
                }
        }

        BeginDrawing();
        ClearBackground((Color){10,10,16,255});
        uiDraw(w,h);
        EndDrawing();
    }

    uiFree();
    CloseWindow();
    return 0;
}
