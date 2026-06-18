#ifndef GAME_H
#define GAME_H

#include <raylib.h>

#define MAX_LOG 100
#define MAX_BUILD 12
#define MAX_TECH 10
#define WIN_POP 100
#define MAX_TURNS 300

typedef struct {
    int food, wood, stone, gold, herbs, tools, knowledge, faith;
    int pop, popCap, morale, health, soldiers;
    int turn, day, season;
    int hunting, farming, building, scouting, researching;
    int farmers, woodcutters, miners, hunters, scholars, priests;
    int buildings[MAX_BUILD];
    int tech[MAX_TECH];
} State;

typedef struct { char msg[256]; Color col; } LogEntry;

extern State S;
extern LogEntry logs[MAX_LOG];
extern int logN;
extern int gameOver, won;
extern int tab; // 0=overview, 1=buildings, 2=tech
extern char actionDesc[256];
extern const char *SEASONS[4];

// Core
void initGame(void);
void passTime(void);

// Actions
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

// Jobs
int assignJob(int job);
int totalAssigned(void);

// Buildings & Tech
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

// Utility
float clamp01(float v);

#endif
