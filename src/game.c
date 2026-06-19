#include "game.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

State S;
LogEntry logs[MAX_LOG];
int logN, gameOver, won, tab;
char actionDesc[256];
CivData civs[MAX_CIVS];
int currentCiv;
Achievement achievements[MAX_ACH];
int achCount;
int autoSave;
int selectedCivIdx;

const char *SEASONS[] = {"Spring","Summer","Autumn","Winter"};

static const char *BNAMES[MAX_BUILD] = {
    "Hut","Farm","Lumber Mill","Quarry","Market","Temple",
    "Library","Barracks","Wall","Workshop","Inn","Dock",
    "Forge","Granary","Sawmill","University","Observatory"
};
static int BCOST[MAX_BUILD][4] = {
    {5,0,3,0},{5,0,0,0},{5,5,0,0},{3,0,5,0},{8,0,3,5},{5,0,10,0},
    {5,0,8,0},{12,10,0,0},{0,0,15,0},{8,5,0,0},{10,0,5,8},{0,8,0,10},
    {8,5,6,3},{10,8,0,0},{0,15,5,2},{5,10,8,5},{10,15,10,8}
};
static const char *BEFFECT[MAX_BUILD] = {
    "+4 pop cap","+2 food/t","+2 wood/t","+2 stone/t",
    "+1 gold/t","+1 faith/t","+1 know/t",
    "Trains soldiers","Raid defense","Tool prod","+1 herbs +2 cap","+2 gold/trade",
    "+1 tools/t, +prod","-1 food consumed/t","+3 wood/t","+2 know/scholar","+1 research spd"
};

static const char *TNAMES[MAX_TECH] = {
    "Agriculture","Mining","Carpentry","Medicine","Metallurgy",
    "Architecture","Navigation","Writing","Calendar","Warfare",
    "Astronomy","Propulsion","Xenology","Transcendence"
};
static int TCOST[MAX_TECH] = {15,15,20,20,30,30,25,15,10,35,50,70,100,200};
static const char *TDESC[MAX_TECH] = {
    "Farms +50%","Mines +50%","Woodcutters +50%","Health x2",
    "Tool crafting","-25% build cost","Trade +50% gold",
    "+1 know/library","Predict seasons","Soldiers x2",
    "Unlocks colonization","Unlocks space travel",
    "Communicate with aliens","Ascend to higher plane"
};

const char *CROP_NAMES[MAX_CROP] = {"Wheat","Corn","Rice","Potato","Herbs"};
const int CROP_COST[MAX_CROP] = {5,8,7,4,3};
const int CROP_DAYS[MAX_CROP] = {3,4,3,3,2};
const int CROP_MIN_Y[MAX_CROP] = {8,14,10,6,4};
const int CROP_MAX_Y[MAX_CROP] = {14,24,18,12,8};
const int CROP_HERB[MAX_CROP] = {0,0,0,0,2};

const char *HUNT_NAMES[MAX_HUNT] = {"Plains","Forest","Mountains","Swamp"};
const char *HUNT_DESC[MAX_HUNT] = {"Safe","Moderate","Risky, big game","Dangerous, herbs"};
const int HUNT_DANGER[MAX_HUNT] = {0,1,3,5};
const int HUNT_MIN_F[MAX_HUNT] = {4,8,14,6};
const int HUNT_MAX_F[MAX_HUNT] = {10,18,28,16};
const int HUNT_HERB_CHANCE[MAX_HUNT] = {5,20,10,40};
const int HUNT_HERB_AMT[MAX_HUNT] = {1,2,3,4};

void addLog(const char *msg, Color col) {
    if (logN>=MAX_LOG) { for(int i=1;i<MAX_LOG;i++)logs[i-1]=logs[i];logN--; }
    strcpy(logs[logN].msg,msg); logs[logN].col=col; logN++;
}
void addLogF(const char *fmt, Color col, ...) {
    char buf[300]; va_list ap; va_start(ap,col); vsnprintf(buf,300,fmt,ap); va_end(ap);
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
int achUnlocked() { int c=0; for(int i=0;i<MAX_ACH;i++)if(achievements[i].unlocked)c++; return c; }

int buildingCost(int b, int idx) {
    int c=BCOST[b][idx];
    if(idx<4 && techKnown(5)) c=c*3/4;
    return c<1&&idx<4?1:c;
}
int canBuild(int b) {
    return S.food>=buildingCost(b,0) && S.wood>=buildingCost(b,1)
        && S.stone>=buildingCost(b,2) && S.gold>=buildingCost(b,3);
}
int canLearn(int t) { return !S.tech[t] && S.knowledge>=TCOST[t]; }

int currentTier(void) {
    if (S.universesReached>0) return 4;
    if (S.galaxiesReached>0) return 3;
    if (S.shipsBuilt>0 || S.colonyCount>0) return 2;
    if (techKnown(6)) return 1;
    return 0;
}
const char *tierName(int t) {
    static const char *n[]={"Planetary","Interplanetary","Interstellar","Universal","Multiversal"};
    return n[t];
}
const char *tierDesc(int t) {
    static const char *d[]={
        "Build your civilization on one world",
        "Colonize other planets in your system",
        "Explore the galaxy, meet other species",
        "Manipulate space and time itself",
        "Travel the multiverse, become eternal"
    };
    return d[t];
}

int cycleCrop(int dir) {
    S.selectedCrop = (S.selectedCrop + dir + MAX_CROP) % MAX_CROP;
    addLogF("Crop: %s (cost %d, %d days)",(Color){100,220,100,255},
        CROP_NAMES[S.selectedCrop], CROP_COST[S.selectedCrop], CROP_DAYS[S.selectedCrop]);
    return S.selectedCrop;
}
int cycleHuntZone(int dir) {
    S.selectedHuntZone = (S.selectedHuntZone + dir + MAX_HUNT) % MAX_HUNT;
    addLogF("Hunt zone: %s - %s",(Color){180,255,100,255},
        HUNT_NAMES[S.selectedHuntZone], HUNT_DESC[S.selectedHuntZone]);
    return S.selectedHuntZone;
}

// ── ACHIEVEMENTS ──
static void defAch(int i, const char *n, const char *d) {
    achievements[i].unlocked=0; strcpy(achievements[i].name,n);
    snprintf(achievements[i].desc,128,"%s",d);
}
static void initAchievements() {
    for(int i=0;i<MAX_ACH;i++) achievements[i].unlocked=0;
    defAch(0,"First Steps","Reach 10 population");
    defAch(1,"Growing Settlement","Reach 25 population");
    defAch(2,"Thriving Town","Reach 50 population");
    defAch(3,"Civilization","Reach 100 population");
    defAch(4,"Builder","Build 5 different buildings");
    defAch(5,"Architect","Build 10 of any building");
    defAch(6,"Scholar","Research 3 technologies");
    defAch(7,"Enlightened","Research all technologies");
    defAch(8,"Wealthy","Accumulate 200 gold");
    defAch(9,"Food King","Accumulate 500 food");
    defAch(10,"Warrior","Train 5 soldiers");
    defAch(11,"Conqueror","Win a war against another civ");
    defAch(12,"Diplomat","Form an alliance");
    defAch(13,"Trader","Complete 10 trades");
    defAch(14,"Explorer","Send scouts 5 times");
    defAch(15,"Colonizer","Establish a colony on another planet");
    defAch(16,"Spacefarer","Build a spaceship");
    defAch(17,"Galactic Explorer","Reach a new galaxy");
    defAch(18,"Universal Being","Manipulate a universe");
    defAch(19,"Transcendent","Reach the multiverse");
    defAch(20,"Survivor","Survive 200 turns");
    defAch(21,"Eternal","Survive 500 turns");
    defAch(22,"Leader","Have 3 active civilizations");
    defAch(23,"Pacifist","Win without ever going to war");
    defAch(24,"Industrialist","Build 5 Workshops");
    defAch(25,"Theocratic","Build 5 Temples");
    defAch(26,"Sage","Accumulate 500 knowledge");
    defAch(27,"Golden Age","Reach 100 morale");
    defAch(28,"Immortal","Reach 100 health");
    defAch(29,"Diverse","Assign every job type at once");
    defAch(30,"Riches","Accumulate 1000 gold");
    defAch(31,"Naming Day","Give your civ a name");
    defAch(32,"Hero","Train 20 soldiers");
    defAch(33,"Merchant","Accumulate 500 gold from trade");
    defAch(34,"Star Lord","Control 3 planets");
    defAch(35,"Multiversal Emperor","Win the game");
}

void checkAchievements() {
    if (S.pop>=10) achievements[0].unlocked=1;
    if (S.pop>=25) achievements[1].unlocked=1;
    if (S.pop>=50) achievements[2].unlocked=1;
    if (S.pop>=100) achievements[3].unlocked=1;
    int bdiff=0; for(int i=0;i<MAX_BUILD;i++)if(S.buildings[i]>0)bdiff++;
    if (bdiff>=5) achievements[4].unlocked=1;
    int btot=0; for(int i=0;i<MAX_BUILD;i++)btot+=S.buildings[i];
    if (btot>=10) achievements[5].unlocked=1;
    int tcnt=0; for(int i=0;i<MAX_TECH;i++)if(S.tech[i])tcnt++;
    if (tcnt>=3) achievements[6].unlocked=1;
    if (tcnt>=MAX_TECH) achievements[7].unlocked=1;
    if (S.gold>=200) achievements[8].unlocked=1;
    if (S.food>=500) achievements[9].unlocked=1;
    if (S.soldiers>=5) achievements[10].unlocked=1;
    if (S.turn>=200) achievements[20].unlocked=1;
    if (S.turn>=500) achievements[21].unlocked=1;
    if (S.morale>=100) achievements[27].unlocked=1;
    if (S.health>=100) achievements[28].unlocked=1;
    int ajobs=(S.farmers>0)+(S.woodcutters>0)+(S.miners>0)+(S.hunters>0)+(S.scholars>0)+(S.priests>0);
    if (ajobs>=6) achievements[29].unlocked=1;
    if (S.gold>=1000) achievements[30].unlocked=1;
    if (S.soldiers>=20) achievements[32].unlocked=1;
    if (S.shipsBuilt>0) achievements[16].unlocked=1;
    if (S.colonyCount>0) achievements[15].unlocked=1;
    if (S.galaxiesReached>0) achievements[17].unlocked=1;
    if (S.universesReached>0) achievements[18].unlocked=1;
    if (won) achievements[35].unlocked=1;
    int bc=0; for(int i=0;i<MAX_CIVS;i++)if(civs[i].alive)bc++;
    if (bc>=3) achievements[22].unlocked=1;
    if (S.buildings[9]>=5) achievements[24].unlocked=1;
    if (S.buildings[5]>=5) achievements[25].unlocked=1;
    if (S.knowledge>=500) achievements[26].unlocked=1;
}

// ── SAVE / LOAD ──
void saveGame() {
    FILE *f=fopen(SAVE_PATH,"wb");
    if(!f)return;
    fwrite(&S,sizeof(State),1,f);
    fwrite(&logN,sizeof(int),1,f);
    fwrite(logs,sizeof(LogEntry),logN,f);
    fwrite(&gameOver,sizeof(int),1,f);
    fwrite(&won,sizeof(int),1,f);
    fwrite(&tab,sizeof(int),1,f);
    fwrite(&currentCiv,sizeof(int),1,f);
    fwrite(civs,sizeof(CivData),MAX_CIVS,f);
    fwrite(achievements,sizeof(Achievement),MAX_ACH,f);
    fwrite(&achCount,sizeof(int),1,f);
    fwrite(actionDesc,256,1,f);
    fclose(f);
}

int loadGame() {
    FILE *f=fopen(SAVE_PATH,"rb");
    if(!f)return 0;
    { size_t r_=fread(&S,sizeof(State),1,f);(void)r_; }
    { size_t r_=fread(&logN,sizeof(int),1,f);(void)r_; }
    if(logN>0&&logN<=MAX_LOG){size_t r_=fread(logs,sizeof(LogEntry),logN,f);(void)r_;}
    { size_t r_=fread(&gameOver,sizeof(int),1,f);(void)r_; }
    { size_t r_=fread(&won,sizeof(int),1,f);(void)r_; }
    { size_t r_=fread(&tab,sizeof(int),1,f);(void)r_; }
    { size_t r_=fread(&currentCiv,sizeof(int),1,f);(void)r_; }
    { size_t r_=fread(civs,sizeof(CivData),MAX_CIVS,f);(void)r_; }
    { size_t r_=fread(achievements,sizeof(Achievement),MAX_ACH,f);(void)r_; }
    { size_t r_=fread(&achCount,sizeof(int),1,f);(void)r_; }
    { size_t r_=fread(actionDesc,256,1,f);(void)r_; }
    fclose(f);
    return 1;
}

// ── RANDOM EVENTS ──
static int rng(int n) { return rand()%n; }

static void rngEvent() {
    int r=rng(100);
    if(r<6){int a=5+rng(15);S.food+=a;addLogF("Wild fruit! +%d food",(Color){100,255,100,255},a);}
    else if(r<11){int a=2+rng(4);S.pop+=a;S.popCap+=a;addLogF("Wanderers join! +%d pop",(Color){180,180,255,255},a);}
    else if(r<15){int a=3+rng(8);S.wood+=a;addLogF("Driftwood. +%d wood",(Color){180,140,80,255},a);}
    else if(r<19){int a=2+rng(5);S.stone+=a;addLogF("Stone found. +%d stone",(Color){160,160,160,255},a);}
    else if(r<23){int a=2+rng(5);S.gold+=a;addLogF("Traders! +%d gold",(Color){255,220,80,255},a);}
    else if(r<27){int a=2+rng(4);S.herbs+=a;addLogF("Herbs gathered. +%d",(Color){100,255,150,255},a);}
    else if(r<30){addLog("Bard lifts spirits!",(Color){200,150,255,255});S.morale+=8;if(S.morale>100)S.morale=100;}
    else if(r<34){int l=1+rng(3);S.pop-=l;if(S.pop<0)S.pop=0;addLogF("Disease! -%d pop",(Color){255,80,80,255},l);}
    else if(r<37){addLog("Rain damages stores!",(Color){160,160,200,255});S.food-=8;if(S.food<0)S.food=0;}
    else if(r<40){addLog("Merchant caravan!",(Color){255,220,180,255});S.gold+=8;S.herbs+=3;}
    else if(r<44&&S.buildings[7]>0){
        if(S.soldiers>0){int k=rng(S.soldiers/2)+1;S.soldiers-=k;if(S.soldiers<0)S.soldiers=0;addLogF("Raid repelled! Lost %d.",(Color){255,200,100,255},k);}
        else{int l=rng(10)+5;S.food-=l/2;S.gold-=l/2;if(S.food<0)S.food=0;if(S.gold<0)S.gold=0;addLog("Raiders stole resources!",(Color){255,60,60,255});}
    }
    else if(r<47){addLog("Sacred spring! Health restored.",(Color){100,255,200,255});S.health+=15;if(S.health>100)S.health=100;}
    else if(r<50&&currentTier()>=1){int a=1+rng(3);S.colonyCount+=a;addLogF("Colony ship returns. +%d colonies",(Color){100,200,255,255},a);}
    else if(r<52&&currentTier()>=2){int a=1+rng(2);S.shipsBuilt+=a;addLogF("New ship constructed. +%d",(Color){180,255,180,255},a);}
}

static void seasonEvent() {
    switch(S.season){
        case 0:addLog("Spring rains nourish the land.",(Color){80,200,80,255});S.food+=S.buildings[1]*3;break;
        case 1:if(rng(3)==0){addLog("Heatwave! Crops suffer.",(Color){255,180,60,255});S.food-=S.food/4;}break;
        case 2:addLog("Autumn harvest abundant.",(Color){220,180,60,255});S.food+=S.farmers*2+S.buildings[1]*4;break;
        case 3:addLog("Winter arrives. Consumption x2.",(Color){160,200,255,255});break;
    }
}

static int jobBonus(int job) {
    int b=1;
    if(job==0&&techKnown(0))b=2;
    if(job==1&&techKnown(2))b=2;
    if(job==2&&techKnown(1))b=2;
    return b;
}

static int toolBonus(void) {
    if (S.tools>0) return 10; // +10% per tool
    return 0;
}

static void addResources() {
    int tb=toolBonus();
    int fBonus=100+tb;
    S.food+= (S.farmers*3*jobBonus(0)+S.buildings[1]*2+(techKnown(0)?S.buildings[1]:0)) * fBonus/100;
    S.wood+=(S.woodcutters*2*jobBonus(1)+S.buildings[2]*2+S.buildings[14]*3) * fBonus/100;
    S.stone+=(S.miners*2*jobBonus(2)+S.buildings[3]*2) * fBonus/100;
    S.gold+=S.buildings[4]+(techKnown(6)?S.buildings[11]*2:0)+(currentTier()>=3?S.galaxiesReached*2:0);
    S.herbs+=S.buildings[10];
    S.knowledge+=S.scholars+S.buildings[6]*(techKnown(7)?2:1)
        + S.buildings[15]*2 + S.buildings[16]
        + (currentTier()>=2?S.galaxiesReached*3:0);
    S.faith+=S.priests+S.buildings[5]+(currentTier()>=4?S.universesReached*5:0);
    // Forge tools production
    S.tools+=S.buildings[12];
}

void passTime() {
    S.turn++; S.day++;
    if(S.day>30){S.day=1;S.season=(S.season+1)%4;seasonEvent();}
    if(S.turn%50==0) checkAchievements();

    int eaten=S.pop*2 - S.buildings[13]; // Granary reduces consumption
    if(eaten<0)eaten=0;
    if(S.season==3)eaten*=2;
    S.food-=eaten;
    if(S.food<0){
        S.food=0;int starve=1+rng(3);S.pop-=starve;if(S.pop<0)S.pop=0;
        S.morale-=12;if(S.morale<0)S.morale=0;
        addLogF("Starvation! %d perished.",(Color){255,60,60,255},starve);
    }

    addResources();

    if(S.food>S.pop*4&&S.morale>50&&S.pop<S.popCap&&rng(3)==0){S.pop++;addLog("A child is born!",(Color){100,255,180,255});}
    if(S.food>S.pop*3){S.morale+=1;if(S.morale>100)S.morale=100;}
    if(S.morale<20&&rng(4)==0&&S.pop>0){S.pop--;addLog("People leave due to low morale.",(Color){255,120,80,255});}

    // Hunt
    if(S.hunting>0){
        S.hunting--;
        int z=S.selectedHuntZone;
        int g = HUNT_MIN_F[z] + rng(HUNT_MAX_F[z]-HUNT_MIN_F[z]+1);
        S.food+=g;
        addLogF("Hunters return from %s. +%d food",(Color){180,255,100,255},HUNT_NAMES[z],g);
        // Danger
        if(HUNT_DANGER[z]>0 && rng(4)<HUNT_DANGER[z]){
            S.health-=HUNT_DANGER[z]*3; if(S.health<0)S.health=0;
            addLogF("Hunter injured in %s!",(Color){255,100,100,255},HUNT_NAMES[z]);
        }
        // Herbs
        if(rng(100)<HUNT_HERB_CHANCE[z]){
            int h=HUNT_HERB_AMT[z]+rng(3);
            S.herbs+=h;
            addLogF("Hunters found %d herbs!",(Color){100,255,150,255},h);
        }
    }
    // Farm
    if(S.farming>0){
        S.farming--;
        int c=S.selectedCrop;
        int y = CROP_MIN_Y[c] + rng(CROP_MAX_Y[c]-CROP_MIN_Y[c]+1);
        S.food+=y;
        if(CROP_HERB[c]>0){ S.herbs+=CROP_HERB[c]+rng(2); }
        addLogF("Harvested %s! +%d food%s",(Color){100,220,100,255},
            CROP_NAMES[c],y,CROP_HERB[c]>0?" +herbs":"");
    }
    if(S.building>0){S.building--;S.popCap+=5;addLog("Housing done! +5 capacity",(Color){180,180,100,255});}
    if(S.scouting>0){
        S.scouting--;int r=rng(100);
        if(r<35){int a=5+rng(15);S.food+=a;addLogF("Scouts find fertile land. +%d food",(Color){100,255,100,255},a);}
        else if(r<55){int a=3+rng(10);S.wood+=a;addLogF("Scouts find grove. +%d wood",(Color){180,140,80,255},a);}
        else if(r<70){int a=3+rng(8);S.stone+=a;addLogF("Scouts find stone. +%d stone",(Color){160,160,160,255},a);}
        else if(r<85){int a=2+rng(5);S.gold+=a;addLogF("Scouts find gold. +%d gold",(Color){255,220,80,255},a);}
        else addLog("Scouts found nothing.",(Color){140,140,140,255});
    }
    if(S.researching>0){S.researching--;S.knowledge+=4+S.scholars+S.buildings[16];addLog("Research advances.",(Color){200,150,255,255});}
    if(S.colonizing>0){
        S.colonizing--;
        S.colonyCount+=2;
        S.popCap+=10;
        addLog("Colony established! +2 colonies, +10 cap",(Color){100,200,255,255});
    }

    if(rng(3)==0)rngEvent();
    if(S.food>S.pop*2){S.health+=2;if(S.health>100)S.health=100;}
    if(S.health<30&&S.herbs>0&&rng(2)==0){S.herbs--;S.health+=15;if(S.health>100)S.health=100;addLog("Herbs heal the sick.",(Color){100,255,150,255});}
    if(techKnown(3)&&S.food>S.pop*2){S.health+=1;if(S.health>100)S.health=100;}

    if(currentTier()>=2) { S.knowledge+=1; }
    if(currentTier()>=3) { S.food+=S.galaxiesReached*2; S.gold+=S.galaxiesReached; }
    if(currentTier()>=4) { S.faith+=S.universesReached*3; }

    if(S.pop>=WIN_POP){won=1;addLog("VICTORY! Population reaches 100!",(Color){255,255,100,255});}
    if(S.pop<=0){gameOver=1;addLog("Your civilization has fallen.",(Color){255,50,50,255});}
    if(S.turn>=MAX_TURNS){won=1;addLog("Your civilization endures for ages!",(Color){255,255,100,255});}

    checkAchievements();
}

// ── ACTIONS ──
void actHunt() {
    int z=S.selectedHuntZone;
    if(S.health<10+(HUNT_DANGER[z]*2)){addLog("Too weak for this zone.",(Color){200,200,200,255});return;}
    if(S.hunting>0||S.farming>0){addLog("Busy! Wait for current action.",(Color){200,200,200,255});return;}
    S.hunting=2;
    strcpy(actionDesc,"Hunters depart for 2 days.");
    addLogF("Send hunters to %s.",(Color){180,255,100,255},HUNT_NAMES[z]);passTime();autoSave=1;
}
void actFarm() {
    int c=S.selectedCrop;
    if(S.food<CROP_COST[c]){addLogF("Need %d food.",(Color){200,200,200,255},CROP_COST[c]);return;}
    if(S.hunting>0||S.farming>0){addLog("Busy! Wait for current action.",(Color){200,200,200,255});return;}
    S.food-=CROP_COST[c];S.farming=CROP_DAYS[c];
    strcpy(actionDesc,"Crops planted.");
    addLogF("Plant %s. %d days.",(Color){100,220,100,255},CROP_NAMES[c],CROP_DAYS[c]);passTime();autoSave=1;
}
void actGift() { if(S.food<S.pop*2){addLog("Not enough food.",(Color){200,200,200,255});return;}S.food-=S.pop*2;S.morale+=12;if(S.morale>100)S.morale=100;int g=(rng(3)==0&&S.pop<S.popCap)?1:0;if(g)S.pop++;addLogF("Gift food! +12 morale%s",(Color){100,255,150,255},g?" +1 pop":"");strcpy(actionDesc,g?"Food gifted! A child joins.":"Food gifted. Morale up.");passTime();autoSave=1;}
void actBuildHomes() { if(S.wood<8){addLog("Need 8 wood.",(Color){200,200,200,255});return;}S.wood-=8;S.building=2;strcpy(actionDesc,"Building homes. +5 cap in 2d.");addLog("Build housing.",(Color){180,180,100,255});passTime();autoSave=1;}
void actTrade() { if(S.wood<5){addLog("Need 5 wood.",(Color){200,200,200,255});return;}S.wood-=5;int g=10+rng(10)+(techKnown(6)?8:0);S.gold+=g;strcpy(actionDesc,"Traded wood for gold.");addLogF("Trade! +%d gold",(Color){255,220,80,255},g);passTime();autoSave=1;}
void actRecruit() { if(S.gold<10){addLog("Need 10 gold.",(Color){200,200,200,255});return;}if(S.pop>=S.popCap){addLog("Need housing!",(Color){200,200,200,255});return;}S.gold-=10;S.pop+=2;strcpy(actionDesc,"Two settlers join.");addLog("Recruited settlers!",(Color){180,180,255,255});passTime();autoSave=1;}
void actScout() { if(S.scouting>0||S.hunting>0||S.farming>0){addLog("Busy!",(Color){200,200,200,255});return;}S.scouting=2;strcpy(actionDesc,"Scouts depart.");addLog("Send scouts.",(Color){180,200,255,255});passTime();autoSave=1;}
void actRest() { S.health+=12;if(S.health>100)S.health=100;S.morale+=4;if(S.morale>100)S.morale=100;strcpy(actionDesc,"Rest. Health and morale recover.");addLog("Rest.",(Color){140,210,255,255});passTime();autoSave=1;}
void actResearch() { if(S.knowledge<5){addLog("Need 5 knowledge.",(Color){200,200,200,255});return;}if(S.researching>0){addLog("Already researching!",(Color){200,200,200,255});return;}S.knowledge-=5;S.researching=3;strcpy(actionDesc,"Researching. 3 days.");addLog("Begin research.",(Color){200,150,255,255});passTime();autoSave=1;}
void actHeal() { if(S.herbs<2){addLog("Need 2 herbs.",(Color){200,200,200,255});return;}S.herbs-=2;int h=15+rng(15);S.health+=h;if(S.health>100)S.health=100;addLogF("Herbal med. +%d health",(Color){100,255,150,255},h);strcpy(actionDesc,"Healed with herbs.");passTime();autoSave=1;}
void actTrain() { if(S.gold<15||S.food<15){addLog("Need 15g 15f.",(Color){200,200,200,255});return;}if(totalAssigned()>=S.pop-S.soldiers){addLog("No spare pop.",(Color){200,200,200,255});return;}S.gold-=15;S.food-=15;S.soldiers++;addLog("Trained soldier!",(Color){255,180,100,255});strcpy(actionDesc,"Soldier trained.");passTime();autoSave=1;}
void actColonize() { if(!techKnown(10)){addLog("Need Astronomy.",(Color){200,200,200,255});return;}if(S.food<30||S.wood<20||S.gold<15){addLog("Need 30f 20w 15g.",(Color){200,200,200,255});return;}S.food-=30;S.wood-=20;S.gold-=15;S.colonizing=3;strcpy(actionDesc,"Colony expedition launched. 3 days.");addLog("Launch colony expedition!",(Color){100,200,255,255});passTime();autoSave=1;}
void actBuildShip() { if(!techKnown(11)){addLog("Need Propulsion.",(Color){200,200,200,255});return;}if(S.gold<50||S.wood<40||S.stone<30||S.food<30){addLog("Need 30f 40w 30s 50g.",(Color){200,200,200,255});return;}S.gold-=50;S.wood-=40;S.stone-=30;S.food-=30;S.shipsBuilt++;addLogF("Starship built! (%d total)",(Color){180,255,180,255},S.shipsBuilt);strcpy(actionDesc,"Starship constructed. Space awaits!");passTime();autoSave=1;}
void actExploreGalaxy() { if(S.shipsBuilt<1){addLog("Need a starship first.",(Color){200,200,200,255});return;}S.shipsBuilt--;S.galaxiesReached++;addLogF("Exploring the galaxy! Galaxies: %d",(Color){180,200,255,255},S.galaxiesReached);strcpy(actionDesc,"Exploring beyond the stars...");passTime();autoSave=1;}

void advanceTier() {
    if(currentTier()==2&&S.galaxiesReached>=3){S.universesReached++;addLog("You perceive the fabric of the universe!",(Color){200,150,255,255});}
    if(currentTier()==3&&S.universesReached>=3){addLog("YOU TRANSCEND THE MULTIVERSE!",(Color){255,255,100,255});won=1;}
}

int assignJob(int job) {
    if(totalAssigned()>=S.pop-S.soldiers){addLog("No idle pop.",(Color){200,200,200,255});return 0;}
    int *jobs[]={&S.farmers,&S.woodcutters,&S.miners,&S.hunters,&S.scholars,&S.priests};
    if(job<0||job>=6)return 0;
    (*jobs[job])++;
    const char *n[]={"farmer","woodcutter","miner","hunter","scholar","priest"};
    addLogF("Assign %s.",(Color){160,200,160,255},n[job]);
    return 1;
}

void buildBuilding(int b) {
    if(b<0||b>=MAX_BUILD)return;
    if(!canBuild(b)){addLog("Not enough resources.",(Color){200,200,200,255});return;}
    S.food-=buildingCost(b,0);S.wood-=buildingCost(b,1);S.stone-=buildingCost(b,2);S.gold-=buildingCost(b,3);
    S.buildings[b]++;
    if(b==0)S.popCap+=4;
    if(b==10)S.popCap+=2;
    // Granary effect applied in passTime
    addLogF("Built %s! (total: %d)",(Color){100,200,255,255},BNAMES[b],S.buildings[b]);
    strcpy(actionDesc,"Construction complete.");
    passTime();autoSave=1;
}

void learnTech(int t) {
    if(t<0||t>=MAX_TECH)return;
    if(S.tech[t]){addLog("Already known.",(Color){200,200,200,255});return;}
    if(S.knowledge<TCOST[t]){addLogF("Need %d knowledge.",(Color){200,200,200,255},TCOST[t]);return;}
    S.knowledge-=TCOST[t];S.tech[t]=1;
    addLogF("Discovered: %s!",(Color){180,130,255,255},TNAMES[t]);
    strcpy(actionDesc,"New technology unlocked!");
    if(t==10) addLog("Astronomy unlocked! You can now colonize other planets.",(Color){100,200,255,255});
    if(t==11) addLog("Propulsion! You can build starships.",(Color){180,255,180,255});
    if(t==12) addLog("Xenology! You can communicate with alien civilizations.",(Color){200,150,255,255});
    if(t==13) addLog("Transcendence! Ultimate knowledge awaits...",(Color){255,255,100,255});
    passTime();autoSave=1;
}

// ── CIVILIZATIONS ──
static void initCiv(int idx, const char *name, int seed) {
    srand(seed);
    CivData *c=&civs[idx];
    memset(&c->s,0,sizeof(State));
    c->s.food=20+rng(30); c->s.wood=10+rng(20); c->s.stone=5+rng(15);
    c->s.gold=3+rng(10); c->s.pop=5+rng(8); c->s.popCap=c->s.pop+5+rng(8);
    c->s.morale=40+rng(30); c->s.health=50+rng(30);
    c->s.buildings[1]=rng(3); c->s.buildings[2]=rng(2); c->s.turn=0; c->s.day=1;
    c->s.farmers=rng(3); c->s.woodcutters=rng(2);
    strcpy(c->name,name);
    c->alive=1; c->discovered=0; c->relation=rng(80)-40; c->tier=0;
    c->warsWon=0; c->warsLost=0;
    srand(time(0));
}

void switchCiv(int idx) {
    if(idx<0||idx>=MAX_CIVS||!civs[idx].alive)return;
    if(!civs[idx].discovered){addLog("Civilization not yet discovered.",(Color){200,200,200,255});return;}
    civs[currentCiv].s=S; civs[currentCiv].tier=currentTier();
    currentCiv=idx; S=civs[idx].s;
    strcpy(actionDesc,"Switched civilization view.");
    addLogF("Now viewing: %s",(Color){200,200,255,255},civs[idx].name);
    autoSave=1;
}

void civTrade(int idx) {
    if(idx<0||idx>=MAX_CIVS||!civs[idx].alive||!civs[idx].discovered)return;
    CivData *c=&civs[idx];
    if(c->relation<-30){addLog("They refuse trade (relations too low).",(Color){255,150,100,255});return;}
    if(S.gold<10){addLog("Need 10 gold for trade.",(Color){200,200,200,255});return;}
    S.gold-=10;
    int gain=10+rng(15)+(techKnown(6)?8:0);
    S.food+=gain/2; S.wood+=gain/3; S.stone+=gain/4;
    c->relation+=5; if(c->relation>100)c->relation=100;
    addLogF("Trade with %s! Resources gained, relation +5",(Color){100,200,255,255},c->name);
    strcpy(actionDesc,"Inter-civilization trade successful.");
    passTime();autoSave=1;
}

void civWar(int idx) {
    if(idx<0||idx>=MAX_CIVS||!civs[idx].alive||!civs[idx].discovered)return;
    if(S.soldiers<3){addLog("Need at least 3 soldiers.",(Color){200,200,200,255});return;}
    CivData *c=&civs[idx];
    int ourPower=S.soldiers*2+(techKnown(9)?3:1);
    int theirPower=c->s.soldiers*2+1+rng(5);
    if(ourPower>theirPower){
        int loot=10+rng(30); S.gold+=loot; S.food+=loot;
        c->relation-=30; if(c->relation<-100)c->relation=-100;
        c->s.pop-=1+rng(3); if(c->s.pop<0)c->s.pop=0;
        c->s.soldiers=0;
        addLogF("Victory over %s! Looted %d resources.",(Color){255,220,80,255},c->name,loot);
        achievements[11].unlocked=1;
        S.soldiers-=rng(S.soldiers/3)+1; if(S.soldiers<0)S.soldiers=0;
        strcpy(actionDesc,"War victory! Resources plundered.");
    } else {
        int loss=rng(10)+5; S.gold-=loss/2; S.food-=loss/2; if(S.gold<0)S.gold=0; if(S.food<0)S.food=0;
        S.soldiers-=1+rng(2); if(S.soldiers<0)S.soldiers=0;
        c->relation-=10; if(c->relation<-100)c->relation=-100;
        addLogF("Defeated by %s! Lost resources.",(Color){255,80,80,255},c->name);
        strcpy(actionDesc,"War defeat. Rebuild and retrain.");
    }
    passTime();autoSave=1;
}

void civAlly(int idx) {
    if(idx<0||idx>=MAX_CIVS||!civs[idx].alive||!civs[idx].discovered)return;
    CivData *c=&civs[idx];
    if(c->relation<20){addLog("Relations too low for alliance.",(Color){200,200,200,255});return;}
    if(S.gold<20){addLog("Need 20 gold for ceremony.",(Color){200,200,200,255});return;}
    S.gold-=20; c->relation+=20; if(c->relation>100)c->relation=100;
    S.morale+=5; if(S.morale>100)S.morale=100;
    addLogF("Alliance formed with %s! Morale +5",(Color){100,255,200,255},c->name);
    achievements[12].unlocked=1;
    strcpy(actionDesc,"Alliance forged between civilizations.");
    passTime();autoSave=1;
}

void simulateAI() {
    for(int i=0;i<MAX_CIVS;i++){
        if(!civs[i].alive||i==currentCiv)continue;
        CivData *c=&civs[i];
        if(rng(4)==0)c->s.food+=1+rng(5);
        if(rng(5)==0)c->s.wood+=1+rng(3);
        if(rng(8)==0)c->s.pop++;
        if(c->s.pop>c->s.popCap&&rng(3)==0)c->s.popCap+=2;
        if(c->s.food<0){c->s.food=0;c->s.pop-=rng(2);if(c->s.pop<0)c->s.pop=0;}
        if(c->s.knowledge>20+rng(30)&&rng(10)==0){c->tier=1;}
        if(c->s.knowledge>50+rng(50)&&c->tier==1&&rng(15)==0){c->tier=2;}
        if(c->discovered&&c->relation>0&&rng(20)==0){
            civs[currentCiv].s.gold+=2+rng(4);
            c->relation+=2;
        }
        c->s.turn++;
    }
}

void initGame() {
    S.food=40; S.wood=20; S.stone=10; S.gold=8; S.herbs=5; S.tools=0; S.knowledge=0; S.faith=0;
    S.pop=8; S.popCap=12; S.morale=60; S.health=80; S.soldiers=0;
    S.turn=0; S.day=1; S.season=0;
    S.hunting=0; S.farming=0; S.building=0; S.scouting=0; S.researching=0; S.colonizing=0;
    S.farmers=0; S.woodcutters=0; S.miners=0; S.hunters=0; S.scholars=0; S.priests=0;
    S.colonyCount=0; S.shipsBuilt=0; S.galaxiesReached=0; S.universesReached=0;
    S.selectedCrop=0; S.selectedHuntZone=0;
    for(int i=0;i<MAX_BUILD;i++)S.buildings[i]=0;
    for(int i=0;i<MAX_TECH;i++)S.tech[i]=0;
    gameOver=0; won=0; logN=0; tab=0; autoSave=0; currentCiv=0; selectedCivIdx=0;
    srand(time(0));
    strcpy(actionDesc,"Build your civilization. Reach 100 population to win.");

    initCiv(0,"Terran",42);
    initCiv(1,"Zorblax",1234);
    initCiv(2,"Luminari",5678);
    initCiv(3,"Kreth",9012);
    initCiv(4,"Omni",3456);
    initCiv(5,"Nexus",7890);
    civs[0].s=S; civs[0].discovered=1;

    initAchievements();

    addLog("===================================",(Color){80,100,120,255});
    addLog("   EMERGENCE  —  Civilization Sim",(Color){220,220,100,255});
    addLog("===================================",(Color){80,100,120,255});
    addLog("Q=Overview  W=Build  E=Tech  R=Civs  A=Achievements",(Color){120,120,150,255});
    addLog("H=Hunt  F=Farm  G=Gift  B=Homes  U=Research",(Color){120,120,150,255});
    addLog("T=Trade  Z=Rest  C=Heal  M=Recruit  S=Scout",(Color){120,120,150,255});
    addLog("P=Train  I=Colonize  Y=Ship  O=Explore",(Color){120,120,150,255});
    addLog("J=Crop  D=Zone  K=Save  L=Load  N=New",(Color){120,120,150,255});
    addLog("1-6=Jobs  [Civs]:0-5=sel T=trade W=war A=ally V=switch",(Color){120,120,150,255});
}
