#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define W 960
#define H 680
#define MAX_LOG 80

typedef struct {
    int food, wood, gold, pop, popCap, morale, health, turn, day;
    int hunting, farming, building, scouting;
} State;

typedef struct { char msg[100]; Color col; } Log;
static Log logs[MAX_LOG];
static int logN;
static State s;
static int gameOver, won;
static char actionDesc[600];

static void addLog(const char *msg, Color col) {
    if (logN >= MAX_LOG) {
        for (int i = 1; i < MAX_LOG; i++) logs[i-1] = logs[i];
        logN--;
    }
    strcpy(logs[logN].msg, msg);
    logs[logN].col = col;
    logN++;
}

static void addLogF(const char *fmt, Color col, ...) {
    char buf[100]; va_list ap; va_start(ap, col); vsnprintf(buf, 100, fmt, ap); va_end(ap);
    addLog(buf, col);
}

static void setDesc(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vsnprintf(actionDesc, 600, fmt, ap); va_end(ap);
}

static void rngEvent() {
    int r = rand() % 100;
    if (r < 8) {
        int amt = 5 + rand() % 15;
        s.food += amt;
        addLogF("Wild fruit found! +%d food", (Color){100,255,100,255}, amt);
    } else if (r < 14) {
        int amt = 2 + rand() % 6;
        s.pop += amt; s.popCap += amt;
        addLogF("Wanderers join the tribe! +%d pop", (Color){180,180,255,255}, amt);
    } else if (r < 19) {
        int amt = 3 + rand() % 8;
        s.wood += amt;
        addLogF("Driftwood washes ashore. +%d wood", (Color){180,140,80,255}, amt);
    } else if (r < 23) {
        int amt = 2 + rand() % 5;
        s.gold += amt;
        addLogF("Traders pass through. +%d gold", (Color){255,220,80,255}, amt);
    } else if (r < 27) {
        addLog("A wandering bard lifts spirits!", (Color){200,150,255,255});
        s.morale += 5; if (s.morale > 100) s.morale = 100;
    } else if (r < 31) {
        int loss = 1 + rand() % 3;
        s.pop -= loss; if (s.pop < 0) s.pop = 0;
        addLogF("Disease sweeps through. -%d population", (Color){255,80,80,255}, loss);
    } else if (r < 35) {
        addLog("Heavy rains damage the stores!", (Color){160,160,200,255});
        s.food -= 5; if (s.food < 0) s.food = 0;
    } else if (r < 38) {
        addLog("A traveling merchant offers wares.", (Color){255,220,180,255});
        s.gold += 8;
    }
}

static void passTime() {
    s.turn++;
    s.day++;
    if (s.day > 30) { s.day = 1; }

    // Consumption
    int eaten = s.pop * 2;
    s.food -= eaten;
    if (s.food < 0) {
        s.food = 0;
        int starve = 1 + rand() % 3;
        s.pop -= starve; if (s.pop < 0) s.pop = 0;
        s.morale -= 10; if (s.morale < 0) s.morale = 0;
        addLogF("Not enough food! %d people starved!", (Color){255,60,60,255}, starve);
    }

    // Population growth
    if (s.food > s.pop * 4 && s.morale > 50 && s.pop < s.popCap) {
        if (rand() % 3 == 0) {
            s.pop++;
            addLog("A child is born! Population grows.", (Color){100,255,180,255});
        }
    }

    // Morale recovery
    if (s.food > s.pop * 3) { s.morale += 1; if (s.morale > 100) s.morale = 100; }
    if (s.morale < 20 && rand() % 4 == 0) {
        int leave = 1 + rand() % 2;
        s.pop -= leave; if (s.pop < 0) s.pop = 0;
        addLogF("People are leaving due to low morale!", (Color){255,120,80,255}, leave);
    }

    // Ongoing actions
    if (s.hunting > 0) {
        s.hunting--;
        int gain = 3 + rand() % 8;
        s.food += gain;
        addLogF("Hunters return with %d food.", (Color){180,255,100,255}, gain);
        if (rand() % 6 == 0) {
            s.health -= 5; if (s.health < 0) s.health = 0;
            addLog("A hunter was injured!", (Color){255,100,100,255});
        }
    }
    if (s.farming > 0) {
        s.farming--;
        int gain = 6 + rand() % 10;
        s.food += gain;
        addLogF("Crops harvested! +%d food", (Color){100,220,100,255}, gain);
    }
    if (s.building > 0) {
        s.building--;
        s.popCap += 3;
        addLog("Shelter completed! Population capacity +3", (Color){180,180,100,255});
    }
    if (s.scouting > 0) {
        s.scouting--;
        int r = rand() % 100;
        if (r < 40) {
            int amt = 5 + rand() % 15;
            s.food += amt;
            addLogF("Scouts find fertile ground. +%d food", (Color){100,255,100,255}, amt);
        } else if (r < 65) {
            int amt = 3 + rand() % 10;
            s.wood += amt;
            addLogF("Scouts discover a grove. +%d wood", (Color){180,140,80,255}, amt);
        } else if (r < 80) {
            int amt = 2 + rand() % 6;
            s.gold += amt;
            addLogF("Scouts find a stream with gold. +%d gold", (Color){255,220,80,255}, amt);
        } else {
            addLog("Scouts return with nothing unusual.", (Color){160,160,160,255});
        }
    }

    // Random event
    if (rand() % 3 == 0) rngEvent();

    // Health recovery
    if (s.food > s.pop * 2) { s.health += 2; if (s.health > 100) s.health = 100; }

    // Win / lose checks
    if (s.pop >= 100) { won = 1; addLog("VICTORY: Your tribe has flourished!", (Color){255,255,100,255}); }
    if (s.pop <= 0) { gameOver = 1; addLog("Your tribe has perished.", (Color){255,50,50,255}); }
    if (s.turn >= 200) { won = 1; addLog("Time passes. Your tribe endures!", (Color){255,255,100,255}); }
}

static void actionHunt() {
    if (s.health < 10) { addLog("Too weak to hunt. Rest first.", (Color){200,200,200,255}); return; }
    s.hunting = 2;
    setDesc("Sent hunters out for 2 days. They will bring food but may face danger.");
    addLog("You send hunters into the wild.", (Color){180,255,100,255});
    passTime();
}

static void actionFarm() {
    if (s.food < 5) { addLog("Not enough seed food to plant!", (Color){200,200,200,255}); return; }
    s.food -= 5;
    s.farming = 3;
    setDesc("Planted crops. They will yield food in 3 days.");
    addLog("You plant crops. (+3 days)", (Color){100,220,100,255});
    passTime();
}

static void actionFeast() {
    if (s.food < 10 + s.pop) { addLog("Not enough food for a feast!", (Color){200,200,200,255}); return; }
    s.food -= 10 + s.pop;
    s.morale += 15; if (s.morale > 100) s.morale = 100;
    setDesc("People feast and celebrate! Morale rises.");
    addLog("You hold a great feast! Morale +15", (Color){255,200,80,255});
    passTime();
}

static void actionBuild() {
    if (s.wood < 8) { addLog("Need 8 wood to build shelter!", (Color){200,200,200,255}); return; }
    s.wood -= 8;
    s.building = 2;
    setDesc("Builders construct new shelter. Capacity +3 in 2 days.");
    addLog("You assign builders. (+2 days)", (Color){180,180,100,255});
    passTime();
}

static void actionTrade() {
    if (s.wood < 5) { addLog("Need 5 wood to trade!", (Color){200,200,200,255}); return; }
    s.wood -= 5;
    s.gold += 8 + rand() % 5;
    setDesc("Traded wood for gold with passing merchants.");
    addLog("You trade wood for gold.", (Color){255,220,80,255});
    passTime();
}

static void actionRecruit() {
    if (s.gold < 10) { addLog("Need 10 gold to recruit!", (Color){200,200,200,255}); return; }
    if (s.pop >= s.popCap) { addLog("Need more shelter before recruiting!", (Color){200,200,200,255}); return; }
    s.gold -= 10;
    s.pop += 2;
    setDesc("Two warriors join your tribe.");
    addLog("You recruit two new members!", (Color){180,180,255,255});
    passTime();
}

static void actionGift() {
    if (s.food < s.pop * 2) { addLog("Not enough food to give away!", (Color){200,200,200,255}); return; }
    int amt = s.pop * 2;
    s.food -= amt;
    s.morale += 10; if (s.morale > 100) s.morale = 100;
    int growth = (rand() % 3 == 0) ? 1 : 0;
    if (growth && s.pop < s.popCap) s.pop++;
    setDesc("You distribute food to the people. Morale rises%s.", growth ? " A new child joins the tribe!" : "");
    addLogF("You gift food to the people! Morale +10%s", (Color){100,255,150,255}, growth ? " (population +1)" : "");
    passTime();
}

static void actionScout() {
    s.scouting = 2;
    setDesc("Scouts depart to explore the surrounding lands.");
    addLog("You send scouts to explore. (+2 days)", (Color){180,200,255,255});
    passTime();
}

static void actionRest() {
    s.health += 8; if (s.health > 100) s.health = 100;
    s.morale += 3; if (s.morale > 100) s.morale = 100;
    setDesc("The tribe rests. Health and morale recover slightly.");
    addLog("You rest for a day.", (Color){160,220,255,255});
    passTime();
}

static void newGame() {
    s.food = 30; s.wood = 15; s.gold = 5;
    s.pop = 8; s.popCap = 12;
    s.morale = 60; s.health = 80;
    s.turn = 0; s.day = 1;
    s.hunting = 0; s.farming = 0;
    s.building = 0; s.scouting = 0;
    gameOver = 0; won = 0;
    logN = 0;
    srand(time(0));
    addLog("============================================", (Color){100,100,120,255});
    addLog("            EMERGENCE - Tribe Simulator", (Color){220,220,100,255});
    addLog("============================================", (Color){100,100,120,255});
    addLog("Guide your tribe to survive and thrive.", (Color){180,180,200,255});
    addLog("Each action advances time. Balance your", (Color){140,140,160,255});
    addLog("resources to reach 100 population!", (Color){140,140,160,255});
    addLog("", (Color){0,0,0,0});
    addLog("H-Hunt  F-Farm  E-Feast  B-Build  T-Trade", (Color){120,120,140,255});
    addLog("R-Recruit  G-Gift  S-Scout  Z-ReST  N-New", (Color){120,120,140,255});
    strcpy(actionDesc, "Press a key to lead your tribe.");
}

int main() {
    InitWindow(W, H, "Emergence - Tribe Simulator");
    SetTargetFPS(20);
    Font font = {0};
    const char *fp[] = {
        "/nix/store/37c8di1dc9zp4xfb1pzqdg1gbpbkniw5-dejavu-fonts-2.37/share/fonts/truetype/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    };
    for (int i = 0; i < 3 && font.texture.id == 0; i++)
        font = LoadFontEx(fp[i], 18, 0, 0);
    if (font.texture.id == 0) font = GetFontDefault();

    newGame();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_N)) newGame();

        if (!gameOver && !won) {
            if (IsKeyPressed(KEY_H)) actionHunt();
            else if (IsKeyPressed(KEY_F)) actionFarm();
            else if (IsKeyPressed(KEY_E)) actionFeast();
            else if (IsKeyPressed(KEY_B)) actionBuild();
            else if (IsKeyPressed(KEY_T)) actionTrade();
            else if (IsKeyPressed(KEY_R)) actionRecruit();
            else if (IsKeyPressed(KEY_G)) actionGift();
            else if (IsKeyPressed(KEY_S)) actionScout();
            else if (IsKeyPressed(KEY_Z)) actionRest();
        }

        BeginDrawing();
        ClearBackground((Color){12,12,18,255});

        // ── Title bar ──
        DrawRectangle(0, 0, W, 32, (Color){20,20,30,255});
        DrawLine(0, 32, W, 32, (Color){40,40,55,255});
        DrawTextEx(font, "EMERGENCE - Tribe Simulator", (Vector2){8, 6}, 16, 1, (Color){200,200,100,255});

        // ── Status panel (left side) ──
        int sx = 10, sy = 44;
        DrawRectangle(sx-4, sy-2, 280, 206, (Color){18,18,26,255});
        DrawRectangleLines(sx-4, sy-2, 280, 206, (Color){35,35,50,255});

        DrawTextEx(font, "TRIBE STATUS", (Vector2){sx, sy}, 14, 1, (Color){160,160,180,255});
        sy += 22;

        char buf[80];
        snprintf(buf, 80, "Population : %d / %d", s.pop, s.popCap);
        DrawTextEx(font, buf, (Vector2){sx, sy}, 14, 1, (Color){200,200,220,255});

        Color popCol = s.morale > 50 ? (Color){100,255,150,255} : (Color){255,200,100,255};
        DrawRectangle(sx+200, sy+2, 60, 10, (Color){30,30,40,255});
        DrawRectangle(sx+200, sy+2, (int)(60 * (s.morale/100.0f)), 10, popCol);
        DrawRectangleLines(sx+200, sy+2, 60, 10, (Color){50,50,65,255});
        DrawTextEx(font, "Morale", (Vector2){sx+265, sy-1}, 11, 1, (Color){130,130,150,255});
        sy += 18;

        Color healthCol = s.health > 50 ? (Color){100,255,150,255} : (Color){255,200,100,255};
        DrawRectangle(sx+200, sy+2, 60, 10, (Color){30,30,40,255});
        DrawRectangle(sx+200, sy+2, (int)(60 * (s.health/100.0f)), 10, healthCol);
        DrawRectangleLines(sx+200, sy+2, 60, 10, (Color){50,50,65,255});
        DrawTextEx(font, "Health", (Vector2){sx+265, sy-1}, 11, 1, (Color){130,130,150,255});
        sy += 20;

        snprintf(buf, 80, "Food  : %d", s.food);
        DrawTextEx(font, buf, (Vector2){sx, sy}, 14, 1, s.food >= s.pop*3 ? (Color){100,220,100,255} : (Color){220,180,100,255});
        snprintf(buf, 80, "Wood  : %d", s.wood);
        DrawTextEx(font, buf, (Vector2){sx+130, sy}, 14, 1, (Color){180,140,80,255});
        sy += 18;
        snprintf(buf, 80, "Gold  : %d", s.gold);
        DrawTextEx(font, buf, (Vector2){sx, sy}, 14, 1, (Color){255,220,80,255});
        snprintf(buf, 80, "Eaten : %d/turn", s.pop*2);
        DrawTextEx(font, buf, (Vector2){sx+130, sy}, 14, 1, (Color){160,120,120,255});
        sy += 20;

        snprintf(buf, 80, "Day %d  Turn %d", s.day, s.turn);
        DrawTextEx(font, buf, (Vector2){sx, sy}, 14, 1, (Color){140,140,180,255});

        if (s.hunting > 0) {
            DrawTextEx(font, ">> Hunting...", (Vector2){sx+130, sy}, 14, 1, (Color){180,255,100,255});
        } else if (s.farming > 0) {
            DrawTextEx(font, ">> Farming...", (Vector2){sx+130, sy}, 14, 1, (Color){100,220,100,255});
        } else if (s.building > 0) {
            DrawTextEx(font, ">> Building...", (Vector2){sx+130, sy}, 14, 1, (Color){180,180,100,255});
        } else if (s.scouting > 0) {
            DrawTextEx(font, ">> Scouting...", (Vector2){sx+130, sy}, 14, 1, (Color){180,200,255,255});
        }

        // ── Action description ──
        sy += 24;
        DrawTextEx(font, "ACTION", (Vector2){sx, sy}, 13, 1, (Color){120,120,160,255});
        sy += 18;
        DrawTextEx(font, actionDesc, (Vector2){sx, sy}, 13, 1, (Color){160,180,200,255});

        // ── Keybinds ──
        sy += 34;
        DrawTextEx(font, "CONTROLS", (Vector2){sx, sy}, 13, 1, (Color){120,120,160,255});
        sy += 18;
        const char *keys[] = {
            "H  Hunt (send hunters, +food in 2 turns)",
            "F  Farm (plant crops, +food in 3 turns)",
            "E  Feast (+morale, costs food)",
            "B  Build shelter (+pop cap, costs wood)",
            "T  Trade wood for gold",
            "R  Recruit (+2 pop, costs gold)",
            "G  Gift food (+morale, may +pop)",
            "S  Scout (explore for resources)",
            "Z  Rest (+health, +morale)",
            "N  New game",
        };
        for (int i = 0; i < 10; i++) {
            DrawTextEx(font, keys[i], (Vector2){sx, sy}, 12, 1, (Color){100,100,130,255});
            sy += 15;
        }

        // ── Message log (right area) ──
        int lx = 310, ly = 44;
        DrawRectangle(lx-4, ly-2, W-lx-6, H-ly-4, (Color){15,15,22,255});
        DrawRectangleLines(lx-4, ly-2, W-lx-6, H-ly-4, (Color){30,30,45,255});

        DrawTextEx(font, "JOURNAL", (Vector2){lx, ly}, 14, 1, (Color){160,160,180,255});
        ly += 24;

        int maxVis = (H - ly - 10) / 16;
        int start = logN > maxVis ? logN - maxVis : 0;
        for (int i = start; i < logN; i++) {
            DrawTextEx(font, logs[i].msg, (Vector2){lx, ly}, 13, 1, logs[i].col);
            ly += 16;
        }

        // ── Win / Lose overlay ──
        if (gameOver) {
            DrawRectangle(0, 0, W, H, (Color){20,10,10,200});
            DrawTextEx(font, "GAME OVER", (Vector2){W/2-100, H/2-30}, 32, 2, (Color){255,50,50,255});
            DrawTextEx(font, "Press N for a new game", (Vector2){W/2-120, H/2+10}, 18, 1, (Color){200,200,200,255});
        }
        if (won) {
            DrawRectangle(0, 0, W, H, (Color){10,20,10,200});
            DrawTextEx(font, "VICTORY", (Vector2){W/2-90, H/2-30}, 32, 2, (Color){100,255,100,255});
            DrawTextEx(font, "Your tribe endures through the ages!", (Vector2){W/2-180, H/2+10}, 18, 1, (Color){200,255,200,255});
            DrawTextEx(font, "Press N for a new game", (Vector2){W/2-120, H/2+35}, 15, 1, (Color){160,200,160,255});
        }

        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
