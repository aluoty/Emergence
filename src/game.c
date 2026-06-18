#include "game.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

State S;
LogEntry logs[MAX_LOG];
int logN;
int gameOver, won;
int tab;
char actionDesc[256];

const char *SEASONS[] = {"Spring","Summer","Autumn","Winter"};
static const char *BNAMES[MAX_BUILD] = {
    "Hut","Farm","Lumber Mill","Quarry","Market","Temple",
    "Library","Barracks","Wall","Workshop","Inn","Dock"
};
static int BCOST[MAX_BUILD][4] = {
    {5,0,3,0},{5,0,0,0},{5,5,0,0},{3,0,5,0},{8,0,3,5},{5,0,10,0},
    {5,0,8,0},{12,10,0,0},{0,0,15,0},{8,5,0,0},{10,0,5,8},{0,8,0,10}
};
static const char *BEFFECT[MAX_BUILD] = {
    "+4 population cap","+2 food/turn passive","+2 wood/turn passive","+2 stone/turn passive",
    "+1 gold/turn passive","+1 faith/turn passive","+1 knowledge/turn passive",
    "Enables soldier training","Raid damage reduction","Tool production",
    "+1 herbs/turn, +2 cap","+2 gold per trade"
};
static const char *TNAMES[MAX_TECH] = {
    "Agriculture","Mining","Carpentry","Medicine","Metallurgy",
    "Architecture","Navigation","Writing","Calendar","Warfare"
};
static int TCOST[MAX_TECH] = {15,15,20,20,30,30,25,15,10,35};
static const char *TDESC[MAX_TECH] = {
    "Farms yield +50%","Mines yield +50%","Woodcutters +50%",
    "Health recovery doubled","Unlocks tool crafting",
    "Buildings cost 25% less","Trade yields +50% gold",
    "+1 knowledge per library","Predict seasons accurately",
    "Soldiers twice as effective"
};

void addLog(const char *msg, Color col) {
    if (logN>=MAX_LOG) { for(int i=1;i<MAX_LOG;i++)logs[i-1]=logs[i];logN--; }
    strcpy(logs[logN].msg,msg); logs[logN].col=col; logN++;
}
static void addLogF(const char *fmt, Color col, ...) {
    char buf[256]; va_list ap; va_start(ap,col); vsnprintf(buf,256,fmt,ap); va_end(ap);
    addLog(buf,col);
}

float clamp01(float v) { return v<0?0:v>1?1:v; }
int totalAssigned() { return S.farmers+S.woodcutters+S.miners+S.hunters+S.scholars+S.priests; }
const char *buildingName(int b) { return BNAMES[b]; }
const char *buildingEffect(int b) { return BEFFECT[b]; }
const char *techName(int t) { return TNAMES[t]; }
const char *techDesc(int t) { return TDESC[t]; }
int techCost(int t) { return TCOST[t]; }
int buildingCount(int b) { return S.buildings[b]; }
int techKnown(int t) { return S.tech[t]; }

int buildingCost(int b, int idx) {
    int c=BCOST[b][idx];
    if (idx<4 && techKnown(5)) c = c*3/4; // Architecture -25%
    return c<1&&idx<4?1:c;
}

int canBuild(int b) {
    return S.food>=buildingCost(b,0) && S.wood>=buildingCost(b,1)
        && S.stone>=buildingCost(b,2) && S.gold>=buildingCost(b,3);
}

int canLearn(int t) {
    return !S.tech[t] && S.knowledge>=TCOST[t];
}

static int rng(int n) { return rand()%n; }
static int jobBonus(int job) {
    int b=1;
    if (job==0&&techKnown(0)) b=2;
    if (job==1&&techKnown(2)) b=2;
    if (job==2&&techKnown(1)) b=2;
    return b;
}

static void addResources() {
    S.food += S.farmers*3*jobBonus(0) + S.buildings[1]*2 + (techKnown(0)?S.buildings[1]:0);
    S.wood += S.woodcutters*2*jobBonus(1) + S.buildings[2]*2;
    S.stone += S.miners*2*jobBonus(2) + S.buildings[3]*2;
    S.gold += S.buildings[4] + (techKnown(6)?S.buildings[11]*2:0);
    S.herbs += S.buildings[10];
    S.knowledge += S.scholars + S.buildings[6]*(techKnown(7)?2:1);
    S.faith += S.priests + S.buildings[5];
}

static void seasonEvent() {
    switch (S.season) {
        case 0: addLog("Spring rains nourish the land.",(Color){80,200,80,255}); S.food+=S.buildings[1]*3; break;
        case 1: if (rng(3)==0) { addLog("Summer heatwave! Crops wither.",(Color){255,180,60,255}); S.food-=S.food/4; } break;
        case 2: addLog("Autumn harvest brings abundance.",(Color){220,180,60,255}); S.food+=S.farmers*2+S.buildings[1]*4; break;
        case 3: addLog("Winter sets in. Consumption doubles.",(Color){160,200,255,255}); break;
    }
}

static void rngEvent() {
    int r=rng(100);
    if (r<6) { int a=5+rng(15);S.food+=a;addLogF("Wild fruit found! +%d food",(Color){100,255,100,255},a); }
    else if (r<11) { int a=2+rng(5);S.pop+=a;S.popCap+=a;addLogF("Wanderers join! +%d pop",(Color){180,180,255,255},a); }
    else if (r<15) { int a=3+rng(8);S.wood+=a;addLogF("Driftwood washes ashore. +%d wood",(Color){180,140,80,255},a); }
    else if (r<19) { int a=2+rng(5);S.stone+=a;addLogF("Stone deposits found. +%d stone",(Color){160,160,160,255},a); }
    else if (r<23) { int a=2+rng(5);S.gold+=a;addLogF("Traders pass through. +%d gold",(Color){255,220,80,255},a); }
    else if (r<27) { int a=2+rng(4);S.herbs+=a;addLogF("Herbs gathered. +%d herbs",(Color){100,255,150,255},a); }
    else if (r<30) { addLog("A wandering bard lifts spirits!",(Color){200,150,255,255});S.morale+=8;if(S.morale>100)S.morale=100; }
    else if (r<34) { int l=1+rng(3);S.pop-=l;if(S.pop<0)S.pop=0;addLogF("Disease strikes! -%d pop",(Color){255,80,80,255},l); }
    else if (r<37) { addLog("Heavy rains damage stores!",(Color){160,160,200,255});S.food-=8;if(S.food<0)S.food=0; }
    else if (r<40) { addLog("Merchant caravan arrives.",(Color){255,220,180,255});S.gold+=8;S.herbs+=3; }
    else if (r<44 && S.buildings[7]>0) {
        if (S.soldiers>0) {
            int k=rng(S.soldiers/2)+1; S.soldiers-=k; if(S.soldiers<0)S.soldiers=0;
            addLogF("Raid repelled! Lost %d soldiers.",(Color){255,200,100,255},k);
        } else {
            int l=rng(15)+5; S.food-=l/2; S.gold-=l/2; if(S.food<0)S.food=0; if(S.gold<0)S.gold=0;
            addLogF("Raiders stole resources!",(Color){255,60,60,255});
        }
    }
    else if (r<47) { addLog("Sacred spring found! Health restored.",(Color){100,255,200,255});S.health+=15;if(S.health>100)S.health=100; }
}

void passTime() {
    S.turn++; S.day++;
    if (S.day>30) { S.day=1; S.season=(S.season+1)%4; seasonEvent(); }

    int eaten=S.pop*2;
    if (S.season==3) eaten*=2;
    S.food-=eaten;
    if (S.food<0) {
        S.food=0; int starve=1+rng(3); S.pop-=starve; if(S.pop<0)S.pop=0;
        S.morale-=12; if(S.morale<0)S.morale=0;
        addLogF("Starvation! %d perished.",(Color){255,60,60,255},starve);
    }

    addResources();

    if (S.food>S.pop*4 && S.morale>50 && S.pop<S.popCap && rng(3)==0) { S.pop++; addLog("A child is born!",(Color){100,255,180,255}); }
    if (S.food>S.pop*3) { S.morale+=1; if(S.morale>100)S.morale=100; }
    if (S.morale<20 && rng(4)==0 && S.pop>0) { S.pop--; addLog("People leave due to low morale.",(Color){255,120,80,255}); }

    if (S.hunting>0) {
        S.hunting--; int g=6+rng(10)+(techKnown(0)?4:0); S.food+=g;
        addLogF("Hunters return. +%d food",(Color){180,255,100,255},g);
        if (S.season==3) { S.health-=10;if(S.health<0)S.health=0; addLog("Hunter caught in blizzard!",(Color){255,100,100,255}); }
        else if (rng(5)==0) { S.health-=5;if(S.health<0)S.health=0; addLog("Hunter injured!",(Color){255,100,100,255}); }
    }
    if (S.farming>0) {
        S.farming--; int g=10+rng(14)+(techKnown(0)?6:0); S.food+=g;
        addLogF("Crops harvested! +%d food",(Color){100,220,100,255},g);
    }
    if (S.building>0) { S.building--; S.popCap+=5; addLog("Housing completed! +5 capacity",(Color){180,180,100,255}); }
    if (S.scouting>0) {
        S.scouting--; int r=rng(100);
        if (r<35) { int a=5+rng(15);S.food+=a;addLogF("Scouts find fertile land. +%d food",(Color){100,255,100,255},a); }
        else if (r<55) { int a=3+rng(10);S.wood+=a;addLogF("Scouts find a grove. +%d wood",(Color){180,140,80,255},a); }
        else if (r<70) { int a=3+rng(8);S.stone+=a;addLogF("Scouts find stone. +%d stone",(Color){160,160,160,255},a); }
        else if (r<85) { int a=2+rng(5);S.gold+=a;addLogF("Scouts find gold. +%d gold",(Color){255,220,80,255},a); }
        else addLog("Scouts found nothing.",(Color){140,140,140,255});
    }
    if (S.researching>0) { S.researching--; S.knowledge+=3+S.scholars; addLog("Research advances. +3 knowledge",(Color){200,150,255,255}); }

    if (rng(3)==0) rngEvent();
    if (S.food>S.pop*2) { S.health+=2; if(S.health>100)S.health=100; }
    if (S.health<30 && S.herbs>0 && rng(2)==0) { S.herbs--; S.health+=15; if(S.health>100)S.health=100; addLog("Herbs used to treat the sick.",(Color){100,255,150,255}); }
    if (techKnown(3) && S.food>S.pop*2) { S.health+=1; if(S.health>100)S.health=100; } // Medicine bonus

    if (S.pop>=WIN_POP) { won=1; addLog("VICTORY! Your civilization flourishes!",(Color){255,255,100,255}); }
    if (S.pop<=0) { gameOver=1; addLog("Your civilization has fallen.",(Color){255,50,50,255}); }
    if (S.turn>=MAX_TURNS) { won=1; addLog("Your civilization endures through the ages!",(Color){255,255,100,255}); }
}

// ── Actions ──
void actHunt() {
    if (S.health<10) { addLog("Too weak to hunt. Rest first.",(Color){200,200,200,255}); return; }
    S.hunting=2; strcpy(actionDesc,"Hunters depart. Food returns in 2 days."); addLog("Send hunters into the wild.",(Color){180,255,100,255}); passTime();
}
void actFarm() {
    if (S.food<5) { addLog("Need 5 food for seed.",(Color){200,200,200,255}); return; }
    S.food-=5; S.farming=3; strcpy(actionDesc,"Crops planted. Harvest in 3 days."); addLog("Plant crops for the season.",(Color){100,220,100,255}); passTime();
}
void actGift() {
    if (S.food< S.pop*2) { addLog("Not enough food to distribute.",(Color){200,200,200,255}); return; }
    S.food-=S.pop*2; S.morale+=12; if(S.morale>100)S.morale=100;
    int g=(rng(3)==0&&S.pop<S.popCap)?1:0; if(g)S.pop++;
    addLogF("Distribute food! Morale +12%s",(Color){100,255,150,255},g?" A child is born!":"");
    strcpy(actionDesc,g?"Food distributed. Population grows!":"Food distributed. Morale improves.");
    passTime();
}
void actBuildHomes() {
    if (S.wood<8) { addLog("Need 8 wood for housing.",(Color){200,200,200,255}); return; }
    S.wood-=8; S.building=2; strcpy(actionDesc,"Building new homes. +5 capacity in 2 days."); addLog("Begin housing construction.",(Color){180,180,100,255}); passTime();
}
void actTrade() {
    if (S.wood<5) { addLog("Need 5 wood to trade.",(Color){200,200,200,255}); return; }
    S.wood-=5; int g=10+rng(10)+(techKnown(6)?8:0); S.gold+=g;
    strcpy(actionDesc,"Merchants exchange wood for gold."); addLogF("Trade completed. +%d gold",(Color){255,220,80,255},g); passTime();
}
void actRecruit() {
    if (S.gold<10) { addLog("Need 10 gold to recruit.",(Color){200,200,200,255}); return; }
    if (S.pop>=S.popCap) { addLog("Need more housing first!",(Color){200,200,200,255}); return; }
    S.gold-=10; S.pop+=2; strcpy(actionDesc,"Two settlers join your civilization."); addLog("Recruited new settlers!",(Color){180,180,255,255}); passTime();
}
void actScout() {
    S.scouting=2; strcpy(actionDesc,"Scouts depart to explore the unknown."); addLog("Send scouts to explore.",(Color){180,200,255,255}); passTime();
}
void actRest() {
    S.health+=12; if(S.health>100)S.health=100; S.morale+=4; if(S.morale>100)S.morale=100;
    strcpy(actionDesc,"The people rest and recover."); addLog("Rest for a day.",(Color){140,210,255,255}); passTime();
}
void actResearch() {
    if (S.knowledge<5) { addLog("Need 5 knowledge to research.",(Color){200,200,200,255}); return; }
    S.knowledge-=5; S.researching=3; strcpy(actionDesc,"Scholars begin researching. 3 days."); addLog("Begin research project.",(Color){200,150,255,255}); passTime();
}
void actHeal() {
    if (S.herbs<2) { addLog("Need 2 herbs for medicine.",(Color){200,200,200,255}); return; }
    S.herbs-=2; int h=15+rng(15); S.health+=h; if(S.health>100)S.health=100;
    addLogF("Herbal medicine heals. +%d health",(Color){100,255,150,255},h); strcpy(actionDesc,"Healed the sick with herbs."); passTime();
}
void actTrain() {
    if (S.gold<15||S.food<15) { addLog("Need 15 gold and 15 food.",(Color){200,200,200,255}); return; }
    if (totalAssigned()>=S.pop-S.soldiers) { addLog("No available population.",(Color){200,200,200,255}); return; }
    S.gold-=15; S.food-=15; S.soldiers++;
    addLog("Trained a soldier.",(Color){255,180,100,255}); strcpy(actionDesc,"Soldier trained. Defense strengthened."); passTime();
}

int assignJob(int job) {
    if (totalAssigned()>=S.pop-S.soldiers) { addLog("No idle population.",(Color){200,200,200,255}); return 0; }
    int *jobs[]={&S.farmers,&S.woodcutters,&S.miners,&S.hunters,&S.scholars,&S.priests};
    if (job<0||job>=6) return 0;
    (*jobs[job])++;
    const char *names[]={"farmer","woodcutter","miner","hunter","scholar","priest"};
    addLogF("Assign %s.",(Color){160,200,160,255},names[job]);
    return 1;
}

void buildBuilding(int b) {
    if (b<0||b>=MAX_BUILD) return;
    if (!canBuild(b)) { addLog("Not enough resources.",(Color){200,200,200,255}); return; }
    S.food-=buildingCost(b,0); S.wood-=buildingCost(b,1); S.stone-=buildingCost(b,2); S.gold-=buildingCost(b,3);
    S.buildings[b]++;
    if (b==0) S.popCap+=4;
    if (b==10) S.popCap+=2;
    addLogF("Built %s! (total: %d)",(Color){100,200,255,255},BNAMES[b],S.buildings[b]);
    strcpy(actionDesc,"Construction complete.");
    passTime();
}

void learnTech(int t) {
    if (t<0||t>=MAX_TECH) return;
    if (S.tech[t]) { addLog("Already researched.",(Color){200,200,200,255}); return; }
    if (S.knowledge<TCOST[t]) { addLogF("Need %d knowledge.",(Color){200,200,200,255},TCOST[t]); return; }
    S.knowledge-=TCOST[t]; S.tech[t]=1;
    addLogF("Discovered: %s!",(Color){180,130,255,255},TNAMES[t]);
    strcpy(actionDesc,"New technology unlocked.");
    passTime();
}

void initGame() {
    S.food=40; S.wood=20; S.stone=10; S.gold=8; S.herbs=5; S.tools=0; S.knowledge=0; S.faith=0;
    S.pop=8; S.popCap=12; S.morale=60; S.health=80; S.soldiers=0;
    S.turn=0; S.day=1; S.season=0;
    S.hunting=0; S.farming=0; S.building=0; S.scouting=0; S.researching=0;
    S.farmers=0; S.woodcutters=0; S.miners=0; S.hunters=0; S.scholars=0; S.priests=0;
    for(int i=0;i<MAX_BUILD;i++)S.buildings[i]=0;
    for(int i=0;i<MAX_TECH;i++)S.tech[i]=0;
    gameOver=0; won=0; logN=0; tab=0;
    strcpy(actionDesc,"Build your civilization. Reach 100 population to win.");
    srand(time(0));
    addLog("================================",(Color){80,100,120,255});
    addLog("  EMERGENCE  Civilization Sim",(Color){220,220,100,255});
    addLog("================================",(Color){80,100,120,255});
    addLog("Q=Overview  W=Buildings  E=Tech  1-6=Assign jobs",(Color){120,120,150,255});
    addLog("H=Hunt  F=Farm  G=Gift  B=Build  R=Research  T=Trade",(Color){120,120,150,255});
    addLog("Z=Rest  C=Heal  M=Recruit  S=Scout  P=Train  N=New",(Color){120,120,150,255});
}
