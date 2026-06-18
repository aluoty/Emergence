#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define MAX_LOG 100
#define MAX_BUILD 12
#define MAX_TECH 10

typedef struct {
    int food, wood, stone, gold, herbs, tools, knowledge, faith;
    int pop, popCap, morale, health, soldiers;
    int turn, day, season;
    int hunting, farming, building, scouting, researching;
    int farmers, woodcutters, miners, hunters, scholars, priests;
    int buildings[MAX_BUILD];
    int tech[MAX_TECH];
    int raidsRepelled;
} State;

typedef struct { char msg[256]; Color col; } Log;
static Log logs[MAX_LOG];
static int logN;
static State s;
static int gameOver, won;
static char actionDesc[256];
static int tab; // 0=info, 1=build, 2=tech

static void addLog(const char *msg, Color col) {
    if (logN >= MAX_LOG) { for (int i=1;i<MAX_LOG;i++) logs[i-1]=logs[i]; logN--; }
    strcpy(logs[logN].msg, msg); logs[logN].col = col; logN++;
}
static void addLogF(const char *fmt, Color col, ...) {
    char buf[256]; va_list ap; va_start(ap,col); vsnprintf(buf,256,fmt,ap); va_end(ap);
    addLog(buf,col);
}

static const char *SEASONS[] = {"Spring","Summer","Autumn","Winter"};
static const char *BNAMES[] = {"Hut","Farm","Lumber Mill","Quarry","Market","Temple","Library","Barracks","Wall","Workshop","Inn","Dock"};
static int BCOST[MAX_BUILD][4] = {
    {5,0,3,0},{5,0,0,0},{5,5,0,0},{3,0,5,0},{8,0,3,5},{5,0,10,0},{5,0,8,0},{12,10,0,0},{0,0,15,0},{8,5,0,0},{10,0,5,8},{0,8,0,10}
};
static const char *TNAMES[] = {"Agriculture","Mining","Carpentry","Medicine","Metallurgy","Architecture","Navigation","Writing","Calendar","Warfare"};
static int TCOST[MAX_TECH] = {15,15,20,20,30,30,25,15,10,35};

static Color hpColor(int v,int hi) {
    float p=v/(float)hi; return p>0.6f?(Color){80,220,80,255}:p>0.3f?(Color){220,200,60,255}:(Color){220,60,60,255};
}
static int rng(int n) { return rand()%n; }
static int hasRes(int f,int w,int st,int g) { return s.food>=f&&s.wood>=w&&s.stone>=st&&s.gold>=g; }
static void spendRes(int f,int w,int st,int g) { s.food-=f; s.wood-=w; s.stone-=st; s.gold-=g; }
static int techKnown(int t) { return s.tech[t]; }
static int bCount(int b) { return s.buildings[b]; }
static int bMod(int b) { return techKnown(0)?(b==1?2:0):0; } // agri boosts farm
static int jobBonus(int job) {
    int bonus=1;
    if (job==0 && techKnown(0)) bonus=2; // farmer + agri
    if (job==1 && techKnown(2)) bonus=2; // woodcutter + carpentry
    if (job==2 && techKnown(1)) bonus=2; // miner + mining
    return bonus;
}

static void addResources() {
    s.food += s.farmers * 3 * jobBonus(0) + bCount(1) * 2 + bMod(1);
    s.wood += s.woodcutters * 2 * jobBonus(1) + bCount(2) * 2;
    s.stone += s.miners * 2 * jobBonus(2) + bCount(3) * 2;
    s.gold += bCount(4) * 1 + (techKnown(6)?bCount(11)*2:0);
    s.herbs += bCount(10) * 1;
    s.knowledge += s.scholars * 1 + bCount(6) * 1;
    s.faith += s.priests * 1 + bCount(5) * 1;
}

static void seasonEvent() {
    switch (s.season) {
        case 0: // spring
            addLog("Spring rains help the crops.",(Color){100,200,100,255});
            s.food += bCount(1)*3;
            break;
        case 1: // summer
            if (rng(3)==0) { addLog("Summer heatwave! Food production suffers.",(Color){255,180,60,255}); s.food-=s.food/4; }
            break;
        case 2: // autumn
            addLog("Autumn harvest is plentiful.",(Color){220,180,60,255});
            s.food += s.farmers*2 + bCount(1)*4;
            break;
        case 3: // winter
            addLog("Winter is harsh. Consumption doubled.",(Color){180,200,255,255});
            break;
    }
}

static void rngEvent() {
    int r=rng(100);
    if (r<6) { int a=5+rng(15); s.food+=a; addLogF("Wild fruit found! +%d food",(Color){100,255,100,255},a); }
    else if (r<11) { int a=2+rng(5); s.pop+=a; s.popCap+=a; addLogF("Wanderers join! +%d pop",(Color){180,180,255,255},a); }
    else if (r<15) { int a=3+rng(8); s.wood+=a; addLogF("Driftwood washes ashore. +%d wood",(Color){180,140,80,255},a); }
    else if (r<19) { int a=2+rng(5); s.stone+=a; addLogF("Stone deposits found. +%d stone",(Color){160,160,160,255},a); }
    else if (r<23) { int a=2+rng(5); s.gold+=a; addLogF("Traders pass through. +%d gold",(Color){255,220,80,255},a); }
    else if (r<27) { int a=2+rng(4); s.herbs+=a; addLogF("Herbs gathered. +%d herbs",(Color){100,255,150,255},a); }
    else if (r<30) { addLog("A wandering bard lifts spirits!",(Color){200,150,255,255}); s.morale+=8;if(s.morale>100)s.morale=100; }
    else if (r<34) { int l=1+rng(3); s.pop-=l;if(s.pop<0)s.pop=0; addLogF("Disease strikes! -%d population",(Color){255,80,80,255},l); }
    else if (r<37) { addLog("Heavy rains damage stores!",(Color){160,160,200,255}); s.food-=8;if(s.food<0)s.food=0; }
    else if (r<40) { addLog("Traders offer fine goods.",(Color){255,220,180,255}); s.gold+=6; s.herbs+=3; }
    else if (r<44 && bCount(7)>0) {
        int loss=rng(bCount(7)*3)+1;
        if (s.soldiers>0) { int killed=rng(s.soldiers/2)+1; s.soldiers-=killed; if(s.soldiers<0)s.soldiers=0; addLogF("Raid repelled! %d soldiers lost.",(Color){255,200,100,255},killed); }
        else { s.food-=loss; if(s.food<0)s.food=0; addLogF("Raiders steal %d food!",(Color){255,60,60,255},loss); }
    }
    else if (r<47) { addLog("Mineral spring discovered! Health boost.",(Color){100,255,200,255}); s.health+=10;if(s.health>100)s.health=100; }
}

static void passTime() {
    s.turn++; s.day++;
    if (s.day>30) { s.day=1; s.season=(s.season+1)%4; seasonEvent(); }

    // consumption
    int eaten = s.pop * 2;
    if (s.season==3) eaten *= 2; // winter
    s.food -= eaten;
    if (s.food<0) {
        s.food=0; int starve=1+rng(3); s.pop-=starve; if(s.pop<0)s.pop=0;
        s.morale-=12; if(s.morale<0)s.morale=0;
        addLogF("Starvation! %d died.",(Color){255,60,60,255},starve);
    }

    // passive resource generation
    addResources();

    // pop growth
    if (s.food>s.pop*4 && s.morale>50 && s.pop<s.popCap && rng(3)==0) {
        s.pop++; addLog("A child is born!",(Color){100,255,180,255});
    }
    if (s.food>s.pop*3) { s.morale+=1; if(s.morale>100)s.morale=100; }
    if (s.morale<20 && rng(4)==0 && s.pop>0) { s.pop--; addLog("People leave due to low morale.",(Color){255,120,80,255}); }

    // ongoing actions
    if (s.hunting>0) {
        s.hunting--; int g=4+rng(10); s.food+=g;
        addLogF("Hunters return. +%d food",(Color){180,255,100,255},g);
        if (s.season==3) { s.health-=8; if(s.health<0)s.health=0; addLog("Hunter caught in blizzard!",(Color){255,100,100,255}); }
        else if (rng(6)==0) { s.health-=5;if(s.health<0)s.health=0; addLog("Hunter injured!",(Color){255,100,100,255}); }
    }
    if (s.farming>0) {
        s.farming--; int g=8+rng(12)+(techKnown(0)?5:0); s.food+=g;
        addLogF("Crops harvested! +%d food",(Color){100,220,100,255},g);
    }
    if (s.building>0) {
        s.building--; s.popCap+=5;
        addLog("Housing completed! +5 capacity",(Color){180,180,100,255});
    }
    if (s.scouting>0) {
        s.scouting--;
        int r=rng(100);
        if (r<35) { int a=5+rng(15); s.food+=a; addLogF("Scouts find fertile land. +%d food",(Color){100,255,100,255},a); }
        else if (r<55) { int a=3+rng(10); s.wood+=a; addLogF("Scouts find a grove. +%d wood",(Color){180,140,80,255},a); }
        else if (r<70) { int a=3+rng(8); s.stone+=a; addLogF("Scouts find stone. +%d stone",(Color){160,160,160,255},a); }
        else if (r<85) { int a=2+rng(5); s.gold+=a; addLogF("Scouts find gold. +%d gold",(Color){255,220,80,255},a); }
        else addLog("Scouts return with nothing.",(Color){160,160,160,255});
    }
    if (s.researching>0) {
        s.researching--;
        s.knowledge += 3 + s.scholars;
        addLog("Research advances. Knowledge +3",(Color){200,150,255,255});
    }

    if (rng(3)==0) rngEvent();
    if (s.food>s.pop*2) { s.health+=2; if(s.health>100)s.health=100; }
    if (s.health<30 && s.herbs>0 && rng(2)==0) { s.herbs--; s.health+=10; if(s.health>100)s.health=100; addLog("Herbs used to treat the sick.",(Color){100,255,150,255}); }

    if (s.pop>=100) { won=1; addLog("VICTORY! Your civilization thrives!",(Color){255,255,100,255}); }
    if (s.pop<=0) { gameOver=1; addLog("Your civilization has fallen.",(Color){255,50,50,255}); }
    if (s.turn>=300) { won=1; addLog("Your civilization endures through ages!",(Color){255,255,100,255}); }
}

// ── Actions ──
static void actHunt() {
    if (s.health<10) { addLog("Too weak to hunt.",(Color){200,200,200,255}); return; }
    s.hunting=2; strcpy(actionDesc,"Hunters out for 2 days."); addLog("Send hunters.",(Color){180,255,100,255}); passTime();
}
static void actFarm() {
    if (s.food<5) { addLog("Need 5 food for seed.",(Color){200,200,200,255}); return; }
    s.food-=5; s.farming=3; strcpy(actionDesc,"Crops planted. 3 days to harvest."); addLog("Plant crops.",(Color){100,220,100,255}); passTime();
}
static void actBuild() {
    if (s.wood<8) { addLog("Need 8 wood.",(Color){200,200,200,255}); return; }
    s.wood-=8; s.building=2; strcpy(actionDesc,"Building homes. +5 cap in 2 days."); addLog("Build housing.",(Color){180,180,100,255}); passTime();
}
static void actTrade() {
    if (s.wood<5) { addLog("Need 5 wood.",(Color){200,200,200,255}); return; }
    s.wood-=5; int g=10+rng(10)+(techKnown(6)?5:0); s.gold+=g;
    strcpy(actionDesc,"Traded wood for gold."); addLogF("Trade complete. +%d gold",(Color){255,220,80,255},g); passTime();
}
static void actRecruit() {
    if (s.gold<10) { addLog("Need 10 gold.",(Color){200,200,200,255}); return; }
    if (s.pop>=s.popCap) { addLog("Need more housing!",(Color){200,200,200,255}); return; }
    s.gold-=10; s.pop+=2; strcpy(actionDesc,"Two settlers join."); addLog("Recruited 2 settlers!",(Color){180,180,255,255}); passTime();
}
static void actGift() {
    if (s.food< s.pop*2) { addLog("Not enough food.",(Color){200,200,200,255}); return; }
    s.food-=s.pop*2; s.morale+=12; if(s.morale>100)s.morale=100;
    int g=(rng(3)==0&&s.pop<s.popCap)?1:0; if(g)s.pop++;
    addLogF("Gift food! Morale+12%s",(Color){100,255,150,255},g?" +1 pop":"");
    strcpy(actionDesc,g?"Food gifted! A child joins the community.":"Food gifted. Morale rises.");
    passTime();
}
static void actScout() {
    s.scouting=2; strcpy(actionDesc,"Scouts depart for 2 days."); addLog("Send scouts.",(Color){180,200,255,255}); passTime();
}
static void actRest() {
    s.health+=10; if(s.health>100)s.health=100; s.morale+=4; if(s.morale>100)s.morale=100;
    strcpy(actionDesc,"Rest. Health and morale recover."); addLog("Rest for a day.",(Color){160,220,255,255}); passTime();
}
static void actResearch() {
    if (s.knowledge<5) { addLog("Need 5 knowledge.",(Color){200,200,200,255}); return; }
    s.knowledge-=5; s.researching=3; strcpy(actionDesc,"Research in progress. 3 days."); addLog("Begin research.",(Color){200,150,255,255}); passTime();
}
static void actHeal() {
    if (s.herbs<2) { addLog("Need 2 herbs.",(Color){200,200,200,255}); return; }
    s.herbs-=2; int h=15+rng(10); s.health+=h; if(s.health>100)s.health=100;
    addLogF("Herbal medicine. +%d health",(Color){100,255,150,255},h); strcpy(actionDesc,"Healed the sick with herbs."); passTime();
}
static void actTrain() {
    if (s.gold<15||s.food<10) { addLog("Need 15 gold, 10 food.",(Color){200,200,200,255}); return; }
    if (s.pop<=s.soldiers) { addLog("No spare population.",(Color){200,200,200,255}); return; }
    s.gold-=15; s.food-=10; s.soldiers++;
    addLog("Trained a soldier.",(Color){255,180,100,255}); strcpy(actionDesc,"Soldier trained. Defense improved."); passTime();
}

static void buildBuilding(int b) {
    if (b<0||b>=MAX_BUILD) return;
    int *c=BCOST[b];
    if (!hasRes(c[0],c[1],c[2],c[3])) { addLog("Not enough resources!",(Color){200,200,200,255}); return; }
    spendRes(c[0],c[1],c[2],c[3]);
    s.buildings[b]++;
    if (b==0) s.popCap+=4; // hut
    if (b==8) {} // wall - passive defense
    addLogF("Built %s! (%d total)",(Color){180,220,255,255},BNAMES[b],s.buildings[b]);
    strcpy(actionDesc,"Building constructed.");
    passTime();
}

static void learnTech(int t) {
    if (t<0||t>=MAX_TECH) return;
    if (s.tech[t]) { addLog("Already known.",(Color){200,200,200,255}); return; }
    if (s.knowledge<TCOST[t]) { addLogF("Need %d knowledge.",(Color){200,200,200,255},TCOST[t]); return; }
    s.knowledge-=TCOST[t]; s.tech[t]=1;
    addLogF("Researched: %s!",(Color){200,150,255,255},TNAMES[t]);
    strcpy(actionDesc,"New technology discovered.");
    passTime();
}

static void newGame() {
    s.food=40; s.wood=20; s.stone=10; s.gold=5; s.herbs=5; s.tools=0; s.knowledge=0; s.faith=0;
    s.pop=8; s.popCap=12; s.morale=60; s.health=80; s.soldiers=0;
    s.turn=0; s.day=1; s.season=0;
    s.hunting=0; s.farming=0; s.building=0; s.scouting=0; s.researching=0;
    s.farmers=0; s.woodcutters=0; s.miners=0; s.hunters=0; s.scholars=0; s.priests=0;
    for (int i=0;i<MAX_BUILD;i++) s.buildings[i]=0;
    for (int i=0;i<MAX_TECH;i++) s.tech[i]=0;
    gameOver=0; won=0; logN=0; tab=0;
    srand(time(0));
    strcpy(actionDesc,"Build your civilization. Reach 100 population.");
    addLog("--[ EMERGENCE: Civilization Simulator ]--",(Color){220,220,100,255});
    addLog("Manage resources, build, research, and grow.",(Color){180,180,200,255});
    addLog("Tab: Q=Info  W=Build  E=Tech    Assign: 1-6",(Color){100,100,130,255});
    addLog("H-Hunt  F-Farm  G-Gift  B-Build  R-Research  T-Trade",(Color){100,100,130,255});
    addLog("Z-Rest  C-Heal  M-Recruit  S-Scout  P-Train  N-New",(Color){100,100,130,255});
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
    for (int i=0;i<3&&font.texture.id==0;i++) font=LoadFontEx(fp[i],22,0,0);
    if (font.texture.id==0) font=GetFontDefault();

    newGame();

    while (!WindowShouldClose()) {
        int w=GetScreenWidth(), h=GetScreenHeight();
        if (IsKeyPressed(KEY_N)) newGame();

        if (!gameOver && !won) {
            if (IsKeyPressed(KEY_Q)) tab=0;
            else if (IsKeyPressed(KEY_W)) tab=1;
            else if (IsKeyPressed(KEY_E)) tab=2;
            else if (IsKeyPressed(KEY_H)) actHunt();
            else if (IsKeyPressed(KEY_F)) actFarm();
            else if (IsKeyPressed(KEY_G)) actGift();
            else if (IsKeyPressed(KEY_B)) actBuild();
            else if (IsKeyPressed(KEY_R)) actResearch();
            else if (IsKeyPressed(KEY_T)) actTrade();
            else if (IsKeyPressed(KEY_Z)) actRest();
            else if (IsKeyPressed(KEY_C)) actHeal();
            else if (IsKeyPressed(KEY_M)) actRecruit();
            else if (IsKeyPressed(KEY_S)) actScout();
            else if (IsKeyPressed(KEY_P)) actTrain();
            else if (IsKeyPressed(KEY_ONE) && s.farmers+s.woodcutters+s.miners+s.hunters+s.scholars+s.priests < s.pop - s.soldiers) { s.farmers++; addLog("Assign farmer.",(Color){160,200,160,255}); }
            else if (IsKeyPressed(KEY_TWO) && s.farmers+s.woodcutters+s.miners+s.hunters+s.scholars+s.priests < s.pop - s.soldiers) { s.woodcutters++; addLog("Assign woodcutter.",(Color){180,160,120,255}); }
            else if (IsKeyPressed(KEY_THREE) && s.farmers+s.woodcutters+s.miners+s.hunters+s.scholars+s.priests < s.pop - s.soldiers) { s.miners++; addLog("Assign miner.",(Color){160,160,180,255}); }
            else if (IsKeyPressed(KEY_FOUR) && s.farmers+s.woodcutters+s.miners+s.hunters+s.scholars+s.priests < s.pop - s.soldiers) { s.hunters++; addLog("Assign hunter.",(Color){180,220,120,255}); }
            else if (IsKeyPressed(KEY_FIVE) && s.farmers+s.woodcutters+s.miners+s.hunters+s.scholars+s.priests < s.pop - s.soldiers) { s.scholars++; addLog("Assign scholar.",(Color){180,160,220,255}); }
            else if (IsKeyPressed(KEY_SIX) && s.farmers+s.woodcutters+s.miners+s.hunters+s.scholars+s.priests < s.pop - s.soldiers) { s.priests++; addLog("Assign priest.",(Color){200,180,255,255}); }
            // Build/Tech shortcuts
            for (int k=KEY_ZERO;k<=KEY_NINE;k++) {
                if (IsKeyPressed(k)) {
                    int idx=k-KEY_ZERO;
                    if (tab==1 && idx<MAX_BUILD) buildBuilding(idx);
                    else if (tab==2 && idx<MAX_TECH) learnTech(idx);
                }
            }
        }

        BeginDrawing();
        ClearBackground((Color){10,10,16,255});

        // ── TOP BAR ──
        DrawRectangle(0,0,w,46,(Color){18,18,28,255});
        DrawLine(0,46,w,46,(Color){40,40,55,255});
        int tx=10;
        DrawTextEx(font,"EMERGENCE",(Vector2){tx,8},20,1,(Color){230,230,100,255}); tx+=140;
        char buf[256];
        snprintf(buf,256,"Food:%d  Wood:%d  Stone:%d  Gold:%d  Herbs:%d  Tools:%d",
            s.food,s.wood,s.stone,s.gold,s.herbs,s.tools);
        DrawTextEx(font,buf,(Vector2){tx,10},16,1,(Color){180,180,200,255}); tx+=500;
        snprintf(buf,256,"Pop:%d/%d  %s  Day:%d  Turn:%d",s.pop,s.popCap,SEASONS[s.season],s.day,s.turn);
        DrawTextEx(font,buf,(Vector2){tx,10},16,1,(Color){140,140,180,255});

        // morale+health mini bars
        DrawRectangle(w-210,6,200,7,(Color){30,30,45,255});
        DrawRectangle(w-210,6,(int)(200*(s.morale/100.0f)),7,hpColor(s.morale,100));
        DrawRectangleLines(w-210,6,200,7,(Color){50,50,65,255});
        DrawTextEx(font,"Morale",(Vector2){w-90,0},11,1,(Color){100,100,130,255});

        DrawRectangle(w-210,16,200,7,(Color){30,30,45,255});
        DrawRectangle(w-210,16,(int)(200*(s.health/100.0f)),7,hpColor(s.health,100));
        DrawRectangleLines(w-210,16,200,7,(Color){50,50,65,255});
        DrawTextEx(font,"Health",(Vector2){w-90,10},11,1,(Color){100,100,130,255});

        DrawRectangle(w-210,26,200,7,(Color){30,30,45,255});
        float kpct=s.knowledge>100?1.0f:s.knowledge/100.0f;
        DrawRectangle(w-210,26,(int)(200*kpct),7,(Color){180,130,220,255});
        DrawRectangleLines(w-210,26,200,7,(Color){50,50,65,255});
        DrawTextEx(font,"Knowledge",(Vector2){w-90,20},11,1,(Color){100,100,130,255});

        DrawRectangle(w-210,36,200,7,(Color){30,30,45,255});
        float fpct=s.faith>100?1.0f:s.faith/100.0f;
        DrawRectangle(w-210,36,(int)(200*fpct),7,(Color){255,200,100,255});
        DrawRectangleLines(w-210,36,200,7,(Color){50,50,65,255});
        DrawTextEx(font,"Faith",(Vector2){w-90,30},11,1,(Color){100,100,130,255});

        // ── TAB BAR ──
        int ty=50;
        DrawRectangle(0,ty,w,28,(Color){14,14,22,255});
        DrawLine(0,ty+28,w,ty+28,(Color){35,35,50,255});
        const char *tabs[]={"Q: OVERVIEW","W: BUILDINGS","E: TECHNOLOGY"};
        for (int i=0;i<3;i++) {
            Color tc=i==tab?(Color){200,200,120,255}:(Color){100,100,140,255};
            DrawTextEx(font,tabs[i],(Vector2){20+i*230,ty+5},15,1,tc);
        }

        int colH=h-ty-28-72;
        int colW=(w-40)/3;

        // ── TAB CONTENT ──
        if (tab==0) {
            // LEFT: Resources
            int lx=12,ly=ty+32;
            DrawRectangle(lx,ly,colW,colH,(Color){16,16,24,255});
            DrawRectangleLines(lx,ly,colW,colH,(Color){35,35,50,255});
            ly+=10; DrawTextEx(font,"== RESOURCES ==",(Vector2){lx+8,ly},17,1,(Color){160,160,190,255}); ly+=30;
            struct {const char*n;int v;int m;} rs[]={
                {"Food",s.food,999},{"Wood",s.wood,999},{"Stone",s.stone,999},{"Gold",s.gold,999},
                {"Herbs",s.herbs,999},{"Tools",s.tools,999},{"Knowledge",s.knowledge,999},{"Faith",s.faith,999},
            };
            for (int i=0;i<8;i++) {
                Color rc=(Color){200,200,220,255};
                if (i==0) rc=s.food>=s.pop*3?(Color){100,220,100,255}:(Color){220,180,100,255};
                snprintf(buf,256,"%s: %d",rs[i].n,rs[i].v);
                DrawTextEx(font,buf,(Vector2){lx+12,ly},16,1,rc); ly+=22;
            }
            ly+=6;
            snprintf(buf,256,"Consumption: %d food/turn%s",s.pop*2,s.season==3?" (x2 winter)":"");
            DrawTextEx(font,buf,(Vector2){lx+12,ly},15,1,s.food>=s.pop*2?(Color){140,180,140,255}:(Color){220,120,120,255});
            ly+=22;
            snprintf(buf,256,"Soldiers: %d",s.soldiers);
            DrawTextEx(font,buf,(Vector2){lx+12,ly},15,1,(Color){255,180,100,255}); ly+=22;
            if (s.hunting>0) { DrawTextEx(font,">> Hunting...",(Vector2){lx+12,ly},15,1,(Color){180,255,100,255}); ly+=20; }
            if (s.farming>0) { DrawTextEx(font,">> Farming...",(Vector2){lx+12,ly},15,1,(Color){100,220,100,255}); ly+=20; }
            if (s.building>0) { DrawTextEx(font,">> Building...",(Vector2){lx+12,ly},15,1,(Color){180,180,100,255}); ly+=20; }
            if (s.scouting>0) { DrawTextEx(font,">> Scouting...",(Vector2){lx+12,ly},15,1,(Color){180,200,255,255}); ly+=20; }
            if (s.researching>0) { DrawTextEx(font,">> Researching...",(Vector2){lx+12,ly},15,1,(Color){200,150,255,255}); ly+=20; }

            // CENTER: Jobs
            int cx=lx+colW+10;
            DrawRectangle(cx,ty+32,colW,colH,(Color){16,16,24,255});
            DrawRectangleLines(cx,ty+32,colW,colH,(Color){35,35,50,255});
            DrawTextEx(font,"== POPULATION ==",(Vector2){cx+8,ty+42},17,1,(Color){160,160,190,255});
            int jy=ty+72;
            struct {const char*n;int*v;int k;} jobs[]={
                {"Farmers (1)", &s.farmers, KEY_ONE},
                {"Woodcutters (2)",&s.woodcutters,KEY_TWO},
                {"Miners (3)",&s.miners,KEY_THREE},
                {"Hunters (4)",&s.hunters,KEY_FOUR},
                {"Scholars (5)",&s.scholars,KEY_FIVE},
                {"Priests (6)",&s.priests,KEY_SIX},
            };
            int assigned=s.farmers+s.woodcutters+s.miners+s.hunters+s.scholars+s.priests;
            int idle=s.pop-s.soldiers-assigned;
            snprintf(buf,256,"Idle: %d  Soldiers: %d",idle>0?idle:0,s.soldiers);
            DrawTextEx(font,buf,(Vector2){cx+10,jy},16,1,idle>0?(Color){160,200,160,255}:(Color){200,160,160,255}); jy+=24;
            for (int i=0;i<6;i++) {
                Color jc=s.farmers+s.woodcutters+s.miners+s.hunters+s.scholars+s.priests < s.pop-s.soldiers ? (Color){200,200,220,255} : (Color){100,100,120,255};
                snprintf(buf,256,"%s: %d",jobs[i].n,*jobs[i].v);
                DrawTextEx(font,buf,(Vector2){cx+12,jy},16,1,jc); jy+=24;
            }
            jy+=8;
            DrawTextEx(font,"== ACTIONS ==",(Vector2){cx+8,jy},17,1,(Color){160,160,190,255}); jy+=28;
            const char* acts[]={
                "H  Hunt (food, 2 days)",
                "F  Farm (food, 3d, -5 food)",
                "G  Gift food (+morale, +pop)",
                "B  Build (+5 cap, 2d, -8 wood)",
                "R  Research (knowledge, 3d)",
                "T  Trade (wood -> gold)",
                "Z  Rest (+health, +morale)",
                "C  Heal (herbs -> health)",
                "M  Recruit (+2 pop, -10 gold)",
                "S  Scout (resources, 2d)",
                "P  Train soldier",
            };
            for (int i=0;i<11;i++) {
                DrawTextEx(font,acts[i],(Vector2){cx+12,jy},14,1,(Color){110,110,140,255}); jy+=18;
            }

            // RIGHT: Journal
            int jx=cx+colW+10;
            DrawRectangle(jx,ty+32,w-jx-12,colH,(Color){14,14,22,255});
            DrawRectangleLines(jx,ty+32,w-jx-12,colH,(Color){35,35,50,255});
            DrawTextEx(font,"== JOURNAL ==",(Vector2){jx+8,ty+42},17,1,(Color){160,160,190,255});
            int ly2=ty+72, maxVis=(colH-40)/17, start=logN>maxVis?logN-maxVis:0;
            for (int i=start;i<logN;i++) {
                DrawTextEx(font,logs[i].msg,(Vector2){jx+10,ly2},15,1,logs[i].col); ly2+=17;
            }
        }
        else if (tab==1) {
            // BUILDINGS tab
            int lx=12,ly=ty+32;
            DrawRectangle(lx,ly,w-24,colH,(Color){16,16,24,255});
            DrawRectangleLines(lx,ly,w-24,colH,(Color){35,35,50,255});
            ly+=12; DrawTextEx(font,"== BUILDINGS (0-9 to build) ==",(Vector2){lx+10,ly},17,1,(Color){160,160,190,255}); ly+=30;
            for (int i=0;i<MAX_BUILD;i++) {
                int *c=BCOST[i];
                Color avail=hasRes(c[0],c[1],c[2],c[3])?(Color){200,200,220,255}:(Color){120,120,140,255};
                snprintf(buf,256,"%d: %s (%d)  Cost: %df %dw %ds %dg  Produces: %s",
                    i,BNAMES[i],s.buildings[i],c[0],c[1],c[2],c[3],
                    i==0?"+4 cap":i==1?"+2 food/t":i==2?"+2 wood/t":i==3?"+2 stone/t":i==4?"+1 gold/t":i==5?"+1 faith/t":i==6?"+1 knowledge/t":i==7?"defense":i==8?"raid protection":i==9?"tools":i==10?"+1 herbs +2 cap":"+2 gold/trade");
                DrawTextEx(font,buf,(Vector2){lx+12,ly},16,1,avail); ly+=22;
            }
        }
        else if (tab==2) {
            // TECHNOLOGY tab
            int lx=12,ly=ty+32;
            DrawRectangle(lx,ly,w-24,colH,(Color){16,16,24,255});
            DrawRectangleLines(lx,ly,w-24,colH,(Color){35,35,50,255});
            ly+=12; DrawTextEx(font,"== TECHNOLOGIES (0-9 to research) ==",(Vector2){lx+10,ly},17,1,(Color){160,160,190,255}); ly+=30;
            for (int i=0;i<MAX_TECH;i++) {
                Color tc=s.tech[i]?(Color){100,200,100,255}:(s.knowledge>=TCOST[i]?(Color){200,200,220,255}:(Color){120,120,140,255});
                const char *desc[]={
                    "Farms yield +50%","Mines yield +50%","Woodcutters +50%","Health recovery x2","Tools & weapons",
                    "Buildings cost -25%","Trade yields bonus","+1 know/library","Seasons predictable","Soldiers more effective"
                };
                snprintf(buf,256,"%d: %s  (%d know) %s %s",i,TNAMES[i],TCOST[i],desc[i],s.tech[i]?"[DONE]":"");
                DrawTextEx(font,buf,(Vector2){lx+12,ly},16,1,tc); ly+=22;
            }
        }

        // ── BOTTOM BAR ──
        int by=h-66;
        DrawRectangle(0,by,w,66,(Color){14,14,22,255});
        DrawLine(0,by,w,by,(Color){35,35,50,255});
        DrawTextEx(font,">",(Vector2){10,by+10},20,1,(Color){100,180,255,255});
        DrawTextEx(font,actionDesc,(Vector2){32,by+12},17,1,(Color){180,200,230,255});

        int bw=w-20;
        DrawRectangle(12,by+38,bw-240,14,(Color){30,30,42,255});
        float fpct2=s.pop>=100?1.0f:s.pop/100.0f;
        DrawRectangle(12,by+38,(int)((bw-240)*fpct2),14,hpColor(s.pop,100));
        DrawRectangleLines(12,by+38,bw-240,14,(Color){55,55,72,255});
        snprintf(buf,256,"Population: %d/100",s.pop);
        DrawTextEx(font,buf,(Vector2){bw-220,by+38},14,1,(Color){200,200,220,255});

        if (gameOver) {
            DrawRectangle(0,0,w,h,(Color){20,10,10,200});
            DrawTextEx(font,"GAME OVER",(Vector2){w/2-140,h/2-40},40,2,(Color){255,50,50,255});
            DrawTextEx(font,"Press N for new game",(Vector2){w/2-130,h/2+20},22,1,(Color){200,200,200,255});
        }
        if (won) {
            DrawRectangle(0,0,w,h,(Color){10,20,10,200});
            DrawTextEx(font,"VICTORY",(Vector2){w/2-120,h/2-40},40,2,(Color){100,255,100,255});
            DrawTextEx(font,"Press N for new game",(Vector2){w/2-130,h/2+20},22,1,(Color){200,255,200,255});
        }

        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
