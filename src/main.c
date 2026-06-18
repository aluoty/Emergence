#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define MAX_LOG 80

typedef struct {
    int food, wood, gold, pop, popCap, morale, health, turn, day;
    int hunting, farming, building, scouting;
} State;

typedef struct { char msg[256]; Color col; } Log;
static Log logs[MAX_LOG];
static int logN;
static State s;
static int gameOver, won;
static char actionDesc[256];

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
    char buf[256]; va_list ap; va_start(ap, col); vsnprintf(buf, 256, fmt, ap); va_end(ap);
    addLog(buf, col);
}

static Color hpColor(int val, int hi) {
    float pct = (float)val / hi;
    if (pct > 0.6f) return (Color){80,220,80,255};
    if (pct > 0.3f) return (Color){220,200,60,255};
    return (Color){220,60,60,255};
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
        addLogF("Wanderers join! +%d population", (Color){180,180,255,255}, amt);
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
        addLogF("Disease sweeps through! -%d population", (Color){255,80,80,255}, loss);
    } else if (r < 35) {
        addLog("Heavy rains damage the stores!", (Color){160,160,200,255});
        s.food -= 5; if (s.food < 0) s.food = 0;
    } else if (r < 38) {
        addLog("A traveling merchant offers wares.", (Color){255,220,180,255});
        s.gold += 8;
    }
}

static void passTime() {
    s.turn++; s.day++;
    if (s.day > 30) s.day = 1;

    int eaten = s.pop * 2;
    s.food -= eaten;
    if (s.food < 0) {
        s.food = 0;
        int starve = 1 + rand() % 3;
        s.pop -= starve; if (s.pop < 0) s.pop = 0;
        s.morale -= 10; if (s.morale < 0) s.morale = 0;
        addLogF("Not enough food! %d starved!", (Color){255,60,60,255}, starve);
    }
    if (s.food > s.pop * 4 && s.morale > 50 && s.pop < s.popCap && rand() % 3 == 0) {
        s.pop++; addLog("A child is born! Population grows.", (Color){100,255,180,255});
    }
    if (s.food > s.pop * 3) { s.morale += 1; if (s.morale > 100) s.morale = 100; }
    if (s.morale < 20 && rand() % 4 == 0 && s.pop > 0) {
        s.pop--; addLog("People are leaving due to low morale!", (Color){255,120,80,255});
    }

    if (s.hunting > 0) {
        s.hunting--;
        int gain = 3 + rand() % 8; s.food += gain;
        addLogF("Hunters return with %d food.", (Color){180,255,100,255}, gain);
        if (rand() % 6 == 0) { s.health -= 5; if (s.health<0) s.health=0; addLog("A hunter was injured!", (Color){255,100,100,255}); }
    }
    if (s.farming > 0) {
        s.farming--;
        int gain = 6 + rand() % 10; s.food += gain;
        addLogF("Crops harvested! +%d food", (Color){100,220,100,255}, gain);
    }
    if (s.building > 0) {
        s.building--; s.popCap += 3;
        addLog("Shelter completed! Capacity +3", (Color){180,180,100,255});
    }
    if (s.scouting > 0) {
        s.scouting--;
        int r = rand() % 100;
        if (r < 40) { int a=5+rand()%15; s.food+=a; addLogF("Scouts find fertile ground. +%d food",(Color){100,255,100,255},a); }
        else if (r < 65) { int a=3+rand()%10; s.wood+=a; addLogF("Scouts discover a grove. +%d wood",(Color){180,140,80,255},a); }
        else if (r < 80) { int a=2+rand()%6; s.gold+=a; addLogF("Scouts find gold. +%d gold",(Color){255,220,80,255},a); }
        else addLog("Scouts return with nothing.", (Color){160,160,160,255});
    }

    if (rand() % 3 == 0) rngEvent();
    if (s.food > s.pop * 2) { s.health += 2; if (s.health > 100) s.health = 100; }

    if (s.pop >= 100) { won=1; addLog("VICTORY: Population reaches 100!", (Color){255,255,100,255}); }
    if (s.pop <= 0) { gameOver=1; addLog("Your civilization has fallen.", (Color){255,50,50,255}); }
    if (s.turn >= 200) { won=1; addLog("Your civilization endures through the ages!", (Color){255,255,100,255}); }
}

static void actHunt() {
    if (s.health < 10) { addLog("Too weak to hunt. Rest first.",(Color){200,200,200,255}); return; }
    s.hunting=2; strcpy(actionDesc,"Hunters depart. Food arriving in 2 days."); addLog("You send hunters into the wild.",(Color){180,255,100,255}); passTime();
}
static void actFarm() {
    if (s.food < 5) { addLog("Need 5 food for seed!",(Color){200,200,200,255}); return; }
    s.food-=5; s.farming=3; strcpy(actionDesc,"Crops planted. Harvest in 3 days."); addLog("You plant crops.",(Color){100,220,100,255}); passTime();
}
static void actFeast() {
    if (s.food < 10 + s.pop) { addLog("Not enough food for a feast!",(Color){200,200,200,255}); return; }
    s.food-=10+s.pop; s.morale+=15; if (s.morale>100) s.morale=100;
    strcpy(actionDesc,"The people feast and celebrate!"); addLog("You hold a feast! Morale +15",(Color){255,200,80,255}); passTime();
}
static void actBuild() {
    if (s.wood < 8) { addLog("Need 8 wood to build!",(Color){200,200,200,255}); return; }
    s.wood-=8; s.building=2; strcpy(actionDesc,"Building housing. Capacity +3 in 2 days."); addLog("You assign builders.",(Color){180,180,100,255}); passTime();
}
static void actTrade() {
    if (s.wood < 5) { addLog("Need 5 wood to trade!",(Color){200,200,200,255}); return; }
    s.wood-=5; s.gold+=8+rand()%5; strcpy(actionDesc,"Wood traded for gold."); addLog("You trade wood for gold.",(Color){255,220,80,255}); passTime();
}
static void actRecruit() {
    if (s.gold < 10) { addLog("Need 10 gold to recruit!",(Color){200,200,200,255}); return; }
    if (s.pop >= s.popCap) { addLog("Need more housing first!",(Color){200,200,200,255}); return; }
    s.gold-=10; s.pop+=2; strcpy(actionDesc,"Two new people join your civilization."); addLog("You recruit two new people!",(Color){180,180,255,255}); passTime();
}
static void actGift() {
    if (s.food < s.pop * 2) { addLog("Not enough food to give!",(Color){200,200,200,255}); return; }
    s.food -= s.pop * 2; s.morale += 10; if (s.morale > 100) s.morale = 100;
    int growth = (rand() % 3 == 0 && s.pop < s.popCap) ? 1 : 0;
    if (growth) s.pop++;
    addLogF("You gift food! Morale +10%s",(Color){100,255,150,255},growth?" A child is born!":"");
    strcpy(actionDesc,growth?"Food distributed. A new child joins the community!":"Food distributed. Morale improves.");
    passTime();
}
static void actScout() {
    s.scouting=2; strcpy(actionDesc,"Scouts depart to explore the lands."); addLog("You send scouts.",(Color){180,200,255,255}); passTime();
}
static void actRest() {
    s.health+=8; if (s.health>100) s.health=100; s.morale+=3; if (s.morale>100) s.morale=100;
    strcpy(actionDesc,"The civilization rests. Health and morale recover."); addLog("You rest for a day.",(Color){160,220,255,255}); passTime();
}

static void newGame() {
    s.food=30; s.wood=15; s.gold=5; s.pop=8; s.popCap=12;
    s.morale=60; s.health=80; s.turn=0; s.day=1;
    s.hunting=0; s.farming=0; s.building=0; s.scouting=0;
    gameOver=0; won=0; logN=0;
    srand(time(0));
    strcpy(actionDesc,"Guide your civilization. Reach 100 population to win.");
    addLog("---[ EMERGENCE: Civilization Simulator ]---",(Color){220,220,100,255});
    addLog("Survive and thrive. Gather resources, grow",(Color){180,180,200,255});
    addLog("your population to 100. Gift food to spur",(Color){140,140,160,255});
    addLog("population growth. Every action costs time.",(Color){140,140,160,255});
    addLog("",(Color){0,0,0,0});
    addLog("H-Hunt  F-Farm  E-Feast  B-Build  T-Trade  R-Recruit  G-Gift  S-Scout  Z-Rest  N-New",(Color){100,100,120,255});
}

int main() {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 800, "Emergence -- Civilization Simulator");
    SetTargetFPS(20);
    Font font = {0};
    const char *fp[] = {
        "/nix/store/37c8di1dc9zp4xfb1pzqdg1gbpbkniw5-dejavu-fonts-2.37/share/fonts/truetype/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    };
    for (int i = 0; i < 3 && font.texture.id == 0; i++)
        font = LoadFontEx(fp[i], 24, 0, 0);
    if (font.texture.id == 0) font = GetFontDefault();

    newGame();

    while (!WindowShouldClose()) {
        int w = GetScreenWidth();
        int h = GetScreenHeight();

        if (IsKeyPressed(KEY_N)) newGame();

        if (!gameOver && !won) {
            if (IsKeyPressed(KEY_H)) actHunt();
            else if (IsKeyPressed(KEY_F)) actFarm();
            else if (IsKeyPressed(KEY_E)) actFeast();
            else if (IsKeyPressed(KEY_B)) actBuild();
            else if (IsKeyPressed(KEY_T)) actTrade();
            else if (IsKeyPressed(KEY_R)) actRecruit();
            else if (IsKeyPressed(KEY_G)) actGift();
            else if (IsKeyPressed(KEY_S)) actScout();
            else if (IsKeyPressed(KEY_Z)) actRest();
        }

        BeginDrawing();
        ClearBackground((Color){12,12,18,255});

        // -- TOP BAR --
        DrawRectangle(0, 0, w, 52, (Color){18,18,28,255});
        DrawLine(0, 52, w, 52, (Color){40,40,55,255});

        int tx = 12;
        DrawTextEx(font, "EMERGENCE", (Vector2){tx, 10}, 22, 1, (Color){230,230,100,255});
        tx += 150;

        char buf[256];
        snprintf(buf,256,"Food:%d  Wood:%d  Gold:%d  Pop:%d/%d  Day:%d  Turn:%d",
            s.food, s.wood, s.gold, s.pop, s.popCap, s.day, s.turn);
        DrawTextEx(font, buf, (Vector2){tx, 14}, 18, 1, (Color){180,180,200,255});

        // -- THREE-COLUMN layout --
        int colW = (w - 40) / 3;
        int colGap = 10;
        int colH = h - 52 - 72;

        // LEFT: Resources
        int lx = 12, ly = 62;
        DrawRectangle(lx, ly, colW, colH, (Color){16,16,24,255});
        DrawRectangleLines(lx, ly, colW, colH, (Color){35,35,50,255});

        ly += 10;
        DrawTextEx(font, "== RESOURCES ==", (Vector2){lx+8, ly}, 18, 1, (Color){160,160,190,255});
        ly += 34;

        struct { const char *label; int val; int max; } rsrc[] = {
            {"Food",   s.food,   999},
            {"Wood",   s.wood,   999},
            {"Gold",   s.gold,   999},
            {"Pop",    s.pop,    s.popCap},
            {"Morale", s.morale, 100},
            {"Health", s.health, 100},
        };
        for (int i = 0; i < 6; i++) {
            Color tc = (Color){200,200,220,255};
            if (i == 0) tc = s.food >= s.pop*3 ? (Color){100,220,100,255} : (Color){220,180,100,255};
            if (i == 3) tc = hpColor(s.morale, 100);

            snprintf(buf,256,"%s: %d", rsrc[i].label, rsrc[i].val);
            DrawTextEx(font, buf, (Vector2){lx+12, ly}, 17, 1, tc);

            if (i >= 4 && rsrc[i].max > 0) {
                Color bc = hpColor(rsrc[i].val, rsrc[i].max);
                DrawRectangle(lx + 130, ly + 2, colW - 150, 16, (Color){30,30,42,255});
                float pct = (float)rsrc[i].val / rsrc[i].max;
                if (pct > 1.0f) pct = 1.0f;
                DrawRectangle(lx + 130, ly + 2, (int)((colW - 150) * pct), 16, bc);
                DrawRectangleLines(lx + 130, ly + 2, colW - 150, 16, (Color){55,55,72,255});
            }
            ly += 30;
        }

        ly += 6;
        snprintf(buf,256,"Consumption: %d food/turn", s.pop*2);
        DrawTextEx(font, buf, (Vector2){lx+12, ly}, 16, 1, s.food>=s.pop*2 ? (Color){140,180,140,255} : (Color){220,120,120,255});
        ly += 24;

        if (s.hunting > 0) { DrawTextEx(font,">> Hunting...",(Vector2){lx+12,ly},16,1,(Color){180,255,100,255}); ly+=22; }
        if (s.farming > 0) { DrawTextEx(font,">> Farming...",(Vector2){lx+12,ly},16,1,(Color){100,220,100,255}); ly+=22; }
        if (s.building > 0) { DrawTextEx(font,">> Building...",(Vector2){lx+12,ly},16,1,(Color){180,180,100,255}); ly+=22; }
        if (s.scouting > 0) { DrawTextEx(font,">> Scouting...",(Vector2){lx+12,ly},16,1,(Color){180,200,255,255}); ly+=22; }

        // CENTER: Commands
        int cx = lx + colW + colGap, cy = 62;
        DrawRectangle(cx, cy, colW, colH, (Color){16,16,24,255});
        DrawRectangleLines(cx, cy, colW, colH, (Color){35,35,50,255});

        cy += 10;
        DrawTextEx(font, "== COMMANDS ==", (Vector2){cx+8, cy}, 18, 1, (Color){160,160,190,255});
        cy += 34;

        const char *cmds[] = {
            "H   Hunt (food in 2 turns)",
            "F   Farm (food in 3 turns, -5 food)",
            "E   Feast (+15 morale, costs food)",
            "B   Build (+3 cap, costs 8 wood)",
            "T   Trade (wood for gold)",
            "R   Recruit (+2 pop, costs 10 gold)",
            "G   Gift food (+morale, may +pop)",
            "S   Scout (explore for resources)",
            "Z   Rest (+health, +morale)",
            "N   New game",
        };
        for (int i = 0; i < 10; i++) {
            DrawTextEx(font, cmds[i], (Vector2){cx+10, cy}, 16, 1, (Color){110,110,140,255});
            cy += 26;
        }

        // RIGHT: Journal
        int jx = cx + colW + colGap, jy = 62;
        int jw = w - jx - 12;
        DrawRectangle(jx, jy, jw, colH, (Color){14,14,22,255});
        DrawRectangleLines(jx, jy, jw, colH, (Color){35,35,50,255});

        jy += 10;
        DrawTextEx(font, "== JOURNAL ==", (Vector2){jx+8, jy}, 18, 1, (Color){160,160,190,255});
        jy += 30;

        int maxVis = (colH - 40) / 18;
        int start = logN > maxVis ? logN - maxVis : 0;
        for (int i = start; i < logN; i++) {
            DrawTextEx(font, logs[i].msg, (Vector2){jx+10, jy}, 16, 1, logs[i].col);
            jy += 18;
        }

        // -- BOTTOM BAR --
        int by = h - 66;
        DrawRectangle(0, by, w, 66, (Color){16,16,24,255});
        DrawLine(0, by, w, by, (Color){35,35,50,255});

        DrawTextEx(font, "> ", (Vector2){12, by+10}, 20, 1, (Color){100,180,255,255});
        DrawTextEx(font, actionDesc, (Vector2){34, by+12}, 18, 1, (Color){180,200,230,255});

        // -- Overlays --
        if (gameOver) {
            DrawRectangle(0,0,w,h,(Color){20,10,10,200});
            DrawTextEx(font, "GAME OVER", (Vector2){w/2-140, h/2-40}, 40, 2, (Color){255,50,50,255});
            DrawTextEx(font, "Press N for a new game", (Vector2){w/2-130, h/2+20}, 22, 1, (Color){200,200,200,255});
        }
        if (won) {
            DrawRectangle(0,0,w,h,(Color){10,20,10,200});
            DrawTextEx(font, "VICTORY", (Vector2){w/2-120, h/2-40}, 40, 2, (Color){100,255,100,255});
            DrawTextEx(font, "Press N for a new game", (Vector2){w/2-130, h/2+20}, 22, 1, (Color){200,255,200,255});
        }

        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
