#ifndef GAME_H
#define GAME_H

#include <raylib.h>
#include <stdio.h>

#define MAX_LOG 120
#define MAX_BUILD 17
#define MAX_TECH 14
#define MAX_CIVS 6
#define MAX_ACH 36
#define MAX_CROP 5
#define MAX_HUNT 4
#define WIN_POP 100
#define MAX_TURNS 500
#define SAVE_PATH "emergence.sav"

typedef struct {
    int food, wood, stone, gold, herbs, tools, knowledge, faith;
    int pop, popCap, morale, health, soldiers;
    int turn, day, season;
    int hunting, farming, building, scouting, researching, colonizing;
    int farmers, woodcutters, miners, hunters, scholars, priests;
    int buildings[MAX_BUILD];
    int tech[MAX_TECH];
    int colonyCount, shipsBuilt, galaxiesReached, universesReached;
    int selectedCrop, selectedHuntZone;
} State;

typedef struct { char msg[256]; Color col; } LogEntry;

typedef struct {
    State s;
    char name[28];
    int alive, discovered, relation, tier;
    int warsWon, warsLost;
} CivData;

typedef struct {
    int unlocked;
    char name[48];
    char desc[128];
} Achievement;

extern State S;
extern LogEntry logs[MAX_LOG];
extern int logN;
extern int gameOver, won;
extern int tab; // 0=overview,1=build,2=tech,3=civs,4=achs
extern char actionDesc[256];

extern CivData civs[MAX_CIVS];
extern int currentCiv;
extern Achievement achievements[MAX_ACH];
extern int achCount;
extern int autoSave;
extern int selectedCivIdx;

extern const char *SEASONS[4];

extern const char *CROP_NAMES[MAX_CROP];
extern const int CROP_COST[MAX_CROP], CROP_DAYS[MAX_CROP];
extern const int CROP_MIN_Y[MAX_CROP], CROP_MAX_Y[MAX_CROP], CROP_HERB[MAX_CROP];

extern const char *HUNT_NAMES[MAX_HUNT], *HUNT_DESC[MAX_HUNT];
extern const int HUNT_DANGER[MAX_HUNT];
extern const int HUNT_MIN_F[MAX_HUNT], HUNT_MAX_F[MAX_HUNT];
extern const int HUNT_HERB_CHANCE[MAX_HUNT], HUNT_HERB_AMT[MAX_HUNT];

void initGame(void);
void passTime(void);
void saveGame(void);
int loadGame(void);

void actHunt(void);
void actFarm(void);
void actGift(void);
void actBuildHomes(void);
void actResearch(void);
void actTrade(void);
void actRest(void);
void actHeal(void);
void actRecruit(void);
void actScout(void);
void actTrain(void);
void actColonize(void);
void actBuildShip(void);
void actExploreGalaxy(void);

void switchCiv(int idx);
void civTrade(int idx);
void civWar(int idx);
void civAlly(int idx);
void simulateAI(void);

int assignJob(int job);
int totalAssigned(void);

void buildBuilding(int b);
void learnTech(int t);
int canBuild(int b);
int canLearn(int t);
const char *buildingName(int b);
const char *buildingEffect(int b);
const char *techName(int t);
const char *techDesc(int t);
int techCost(int t);
int buildingCost(int b, int idx);
int buildingCount(int b);
int techKnown(int t);

int currentTier(void);
const char *tierName(int t);
const char *tierDesc(int t);
void advanceTier(void);

void checkAchievements(void);
int achUnlocked(void);

float clamp01(float v);
void addLog(const char *msg, Color col);
void addLogF(const char *fmt, Color col, ...);

int cycleCrop(int dir);
int cycleHuntZone(int dir);

#endif
