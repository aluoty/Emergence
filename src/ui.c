#include "game.h"
#include <stdio.h>
#include <string.h>

static Font font;

static Color hex(int r,int g,int b,int a) { return (Color){r,g,b,a}; }
static Color hpColor(int v,int hi) {
    float p=v/(float)hi;
    return p>0.6f?hex(80,220,80,255):p>0.3f?hex(220,200,60,255):hex(220,60,60,255);
}

void uiInit(void) {
    const char *fp[]={
        "/nix/store/37c8di1dc9zp4xfb1pzqdg1gbpbkniw5-dejavu-fonts-2.37/share/fonts/truetype/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
    };
    for(int i=0;i<3&&font.texture.id==0;i++) font=LoadFontEx(fp[i],22,0,0);
    if(font.texture.id==0)font=GetFontDefault();
}

void uiFree(void) { UnloadFont(font); }

static void bar(int x,int y,int w,int h,float pct,Color fg,Color bg,Color border) {
    DrawRectangle(x,y,w,h,bg);
    if(pct>0)DrawRectangle(x,y,(int)(w*clamp01(pct)),h,fg);
    DrawRectangleLines(x,y,w,h,border);
}

static void panel(int x,int y,int w,int h,const char *title,Color titleCol) {
    DrawRectangle(x,y,w,h,hex(16,16,24,255));
    DrawRectangleLines(x,y,w,h,hex(40,40,55,255));
    if(title) {
        DrawRectangle(x+1,y+1,w-2,26,hex(22,22,34,255));
        DrawTextEx(font,title,(Vector2){x+10,y+4},16,1,titleCol);
    }
}

static void drawTopBar(int w) {
    DrawRectangle(0,0,w,48,hex(20,20,32,255));
    DrawLine(0,48,w,48,hex(45,45,60,255));

    int tx=12;
    DrawTextEx(font,"EMERGENCE",(Vector2){tx,10},22,1,hex(230,230,100,255)); tx+=150;

    char buf[256];
    snprintf(buf,256,"Food:%d  Wood:%d  Stone:%d  Gold:%d  Herbs:%d",
        S.food,S.wood,S.stone,S.gold,S.herbs);
    DrawTextEx(font,buf,(Vector2){tx,10},16,1,hex(180,180,210,255)); tx+=520;

    Color sc=S.season==3?hex(180,220,255,255):S.season==0?hex(100,220,100,255):hex(200,200,200,255);
    snprintf(buf,256,"Pop:%d/%d  %s  D%d T%d",S.pop,S.popCap,SEASONS[S.season],S.day,S.turn);
    DrawTextEx(font,buf,(Vector2){tx,10},16,1,sc);
}

static void drawMiniBars(int w) {
    int bx=w-215;
    bar(bx,4,200,8,S.morale/100.0f,hpColor(S.morale,100),hex(25,25,40,255),hex(50,50,65,255));
    DrawTextEx(font,"Morale",(Vector2){w-100,-1},10,1,hex(100,100,130,255));
    bar(bx,15,200,8,S.health/100.0f,hpColor(S.health,100),hex(25,25,40,255),hex(50,50,65,255));
    DrawTextEx(font,"Health",(Vector2){w-100,10},10,1,hex(100,100,130,255));
    bar(bx,26,200,8,clamp01(S.knowledge/100.0f),hex(180,130,220,255),hex(25,25,40,255),hex(50,50,65,255));
    DrawTextEx(font,"Knowledge",(Vector2){w-100,21},10,1,hex(100,100,130,255));
    bar(bx,37,200,8,clamp01(S.faith/100.0f),hex(255,200,100,255),hex(25,25,40,255),hex(50,50,65,255));
    DrawTextEx(font,"Faith",(Vector2){w-100,32},10,1,hex(100,100,130,255));
}

static void drawTabs(int w) {
    int ty=52;
    DrawRectangle(0,ty,w,30,hex(14,14,22,255));
    DrawLine(0,ty+30,w,ty+30,hex(40,40,55,255));
    const char *tabs[]={"Q: OVERVIEW","W: BUILDINGS","E: TECHNOLOGY","R: CIVS","A: ACHIEVE"};
    int tw=155;
    for(int i=0;i<5;i++) {
        int x=12+i*tw;
        if(i==tab) {
            DrawRectangle(x,ty,tw-4,30,hex(25,25,38,255));
            DrawLine(x,ty+30,x+tw-4,ty+30,hex(220,220,100,255));
        }
        Color tc=i==tab?hex(230,230,150,255):hex(120,120,150,255);
        DrawTextEx(font,tabs[i],(Vector2){x+4,ty+6},16,1,tc);
    }
}

static void drawOverview(int x,int y,int w,int h) {
    int colW=(w-24)/3;
    int colH=h-8;

    // ── LEFT: Resources & Tier ──
    panel(x,y,colW,colH,"RESOURCES",hex(160,200,255,255));
    int ly=y+32;
    struct{const char*n;int v;int m;}rs[]={
        {"Food",S.food,999},{"Wood",S.wood,999},{"Stone",S.stone,999},{"Gold",S.gold,999},
        {"Herbs",S.herbs,999},{"Tools",S.tools,999},{"Knowledge",S.knowledge,999},{"Faith",S.faith,999},
    };
    for(int i=0;i<8;i++) {
        Color rc=hex(200,200,220,255);
        if(i==0)rc=S.food>=S.pop*3?hex(100,220,100,255):hex(220,180,100,255);
        char buf[128]; snprintf(buf,128,"%s: %d",rs[i].n,rs[i].v);
        DrawTextEx(font,buf,(Vector2){x+14,ly},16,1,rc); ly+=22;
    }
    ly+=2;
    char buf[256];
    snprintf(buf,256,"Consume: %d food/turn%s",S.pop*2,S.season==3?" (x2)":"");
    Color cc=S.food>=S.pop*2?hex(140,200,140,255):hex(220,120,120,255);
    DrawTextEx(font,buf,(Vector2){x+14,ly},15,1,cc); ly+=22;
    snprintf(buf,256,"Soldiers: %d  Know: %d  Faith: %d",S.soldiers,S.knowledge,S.faith);
    DrawTextEx(font,buf,(Vector2){x+14,ly},15,1,hex(180,180,200,255)); ly+=22;

    // Tier info
    int t=currentTier();
    snprintf(buf,256,"[%s] %s",tierName(t),tierDesc(t));
    DrawTextEx(font,buf,(Vector2){x+14,ly},15,1,hex(180,130,255,255)); ly+=18;
    snprintf(buf,256,"Colonies: %d  Ships: %d",S.colonyCount,S.shipsBuilt);
    DrawTextEx(font,buf,(Vector2){x+14,ly},14,1,hex(140,180,220,255)); ly+=18;
    if(S.galaxiesReached>0){snprintf(buf,256,"Galaxies explored: %d",S.galaxiesReached);DrawTextEx(font,buf,(Vector2){x+14,ly},14,1,hex(160,200,255,255));ly+=16;}
    if(S.universesReached>0){snprintf(buf,256,"Universes reached: %d",S.universesReached);DrawTextEx(font,buf,(Vector2){x+14,ly},14,1,hex(200,150,255,255));ly+=16;}

    const char *ongoing[7]; int oc=0;
    if(S.hunting>0)ongoing[oc++]=">> Hunting...";
    if(S.farming>0)ongoing[oc++]=">> Farming...";
    if(S.building>0)ongoing[oc++]=">> Building...";
    if(S.scouting>0)ongoing[oc++]=">> Scouting...";
    if(S.researching>0)ongoing[oc++]=">> Researching...";
    if(S.colonizing>0)ongoing[oc++]=">> Colonizing...";
    Color ocols[]={hex(180,255,100,255),hex(100,220,100,255),hex(180,180,100,255),hex(180,200,255,255),hex(200,150,255,255),hex(100,200,255,255)};
    if(oc>0){ly+=4;DrawTextEx(font,"-- Ongoing --",(Vector2){x+14,ly},14,1,hex(140,140,170,255));ly+=18;}
    for(int i=0;i<oc;i++) { DrawTextEx(font,ongoing[i],(Vector2){x+14,ly},15,1,ocols[i]); ly+=20; }

    // ── CENTER: Population & Jobs ──
    int cx=x+colW+12;
    panel(cx,y,colW,colH,"POPULATION & ACTIONS",hex(160,200,255,255));
    int jy=y+32;
    int idle=S.pop-S.soldiers-totalAssigned();
    snprintf(buf,256,"Pop: %d/%d  Idle: %d  Soldiers: %d",S.pop,S.popCap,idle>0?idle:0,S.soldiers);
    DrawTextEx(font,buf,(Vector2){cx+14,jy},16,1,idle>0?hex(160,220,160,255):hex(220,160,160,255)); jy+=24;

    struct{const char*n;int*v;}jobs[]={
        {"Farmer [1]",&S.farmers},{"Woodcutter [2]",&S.woodcutters},{"Miner [3]",&S.miners},
        {"Hunter [4]",&S.hunters},{"Scholar [5]",&S.scholars},{"Priest [6]",&S.priests},
    };
    for(int i=0;i<6;i++) {
        snprintf(buf,128,"%s: %d",jobs[i].n,*jobs[i].v);
        Color jc=totalAssigned()<S.pop-S.soldiers?hex(200,200,220,255):hex(100,100,120,255);
        DrawTextEx(font,buf,(Vector2){cx+14,jy},15,1,jc); jy+=21;
    }
    jy+=4;
    DrawTextEx(font,"-- ACTIONS --",(Vector2){cx+14,jy},15,1,hex(140,140,170,255)); jy+=22;
    const char*acts[]={
        "H Hunt  F Farm  G Gift  B Homes  U Research",
        "T Trade  Z Rest  C Heal  M Recruit",
        "S Scout  P Train  I Colonize  Y Ship",
        "O Explore  K Save  L Load  N New",
    };
    for(int i=0;i<4;i++){DrawTextEx(font,acts[i],(Vector2){cx+14,jy},14,1,hex(120,120,150,255));jy+=20;}

    // ── RIGHT: Journal ──
    int jx=cx+colW+12;
    panel(jx,y,w-jx-x,colH,"JOURNAL",hex(160,200,255,255));
    int ly2=y+32, maxVis=(colH-20)/17, start=logN>maxVis?logN-maxVis:0;
    for(int i=start;i<logN;i++){DrawTextEx(font,logs[i].msg,(Vector2){jx+14,ly2},15,1,logs[i].col);ly2+=17;}
}

static void drawBuildings(int x,int y,int w,int h) {
    panel(x,y,w-8,h,"BUILDINGS  [0-9 to build]",hex(160,200,255,255));
    int ly=y+34;
    char buf[256];
    for(int i=0;i<MAX_BUILD;i++) {
        Color avail=canBuild(i)?hex(200,220,240,255):hex(120,120,140,255);
        Color ct=S.buildings[i]>0?hex(100,200,255,255):avail;
        snprintf(buf,256,"%d: %s  [%d]  %s  ",i,buildingName(i),S.buildings[i],buildingEffect(i));
        DrawTextEx(font,buf,(Vector2){x+14,ly},16,1,ct);
        char cost[64]; int pos=0;
        int vals[4]={buildingCost(i,0),buildingCost(i,1),buildingCost(i,2),buildingCost(i,3)};
        const char *labels[4]={"F","W","S","G"};
        for(int j=0;j<4;j++)if(vals[j]>0){char tmp[16];snprintf(tmp,16,"%s%d ",labels[j],vals[j]);strcpy(cost+pos,tmp);pos+=strlen(tmp);}
        DrawTextEx(font,cost,(Vector2){x+14+MeasureTextEx(font,buf,16,1).x+4,ly},15,1,S.buildings[i]>0?hex(80,180,80,255):hex(140,140,160,255));
        ly+=22;
    }
}

static void drawTech(int x,int y,int w,int h) {
    panel(x,y,w-8,h,"TECHNOLOGY  [0-9 to research]",hex(160,200,255,255));
    int ly=y+34;
    char buf[256];
    for(int i=0;i<MAX_TECH;i++) {
        Color tc=S.tech[i]?hex(60,180,60,255):(S.knowledge>=techCost(i)?hex(200,220,240,255):hex(120,120,140,255));
        const char*done=S.tech[i]?" [KNOWN]":"";
        snprintf(buf,256,"%d: %-14s  (%d know)  %s%s",i,techName(i),techCost(i),techDesc(i),done);
        DrawTextEx(font,buf,(Vector2){x+14,ly},16,1,tc);
        ly+=22;
    }
}

static void drawCivs(int x,int y,int w,int h) {
    panel(x,y,w-8,h,"CIVILIZATIONS  [0-5=select T=trade W=war V=switch A=ally]",hex(160,200,255,255));
    int ly=y+34;
    char buf[256];
    for(int i=0;i<MAX_CIVS;i++) {
        CivData *c=&civs[i];
        Color discCol=c->discovered?hex(200,200,220,255):hex(100,100,120,255);
        Color selCol=(i==selectedCivIdx)?hex(255,220,100,255):discCol;
        const char *status=c->alive?"Alive":"Fallen";
        const char *disc=c->discovered?"":" [Hidden]";
        snprintf(buf,256,"%d: %s  %s%s  (Rel:%d Pop:%d Tier:%d)",i,c->name,status,disc,c->relation,c->s.pop,c->tier);
        if(i==currentCiv) DrawTextEx(font,"<< YOU >>",(Vector2){x+14+MeasureTextEx(font,buf,16,1).x+8,ly},14,1,hex(100,200,255,255));
        DrawTextEx(font,buf,(Vector2){x+14,ly},16,1,i==selectedCivIdx?selCol:discCol);
        ly+=24;
    }
    ly+=8;
    DrawTextEx(font,"-- Selected --",(Vector2){x+14,ly},15,1,hex(140,140,170,255)); ly+=22;
    if(civs[selectedCivIdx].alive && civs[selectedCivIdx].discovered) {
        CivData *c=&civs[selectedCivIdx];
        snprintf(buf,256,"%s: Pop %d  Food %d  Wood %d  Stone %d  Gold %d",c->name,c->s.pop,c->s.food,c->s.wood,c->s.stone,c->s.gold);
        DrawTextEx(font,buf,(Vector2){x+14,ly},15,1,hex(180,200,230,255)); ly+=20;
        snprintf(buf,256,"Soldiers %d  Morale %d  Health %d  Relation %d",c->s.soldiers,c->s.morale,c->s.health,c->relation);
        DrawTextEx(font,buf,(Vector2){x+14,ly},15,1,hex(180,200,230,255)); ly+=20;
        if(selectedCivIdx!=currentCiv) {
            DrawTextEx(font,"T = Trade  W = War  V = Switch View  A = Ally",(Vector2){x+14,ly},14,1,hex(100,180,255,255)); ly+=18;
        }
    } else {
        DrawTextEx(font,"Civilization not yet discovered.",(Vector2){x+14,ly},15,1,hex(120,120,140,255));
    }
}

static void drawAchievements(int x,int y,int w,int h) {
    panel(x,y,w-8,h,"ACHIEVEMENTS",hex(160,200,255,255));
    int ly=y+34;
    char buf[256];
    int shown=0;
    for(int i=0;i<MAX_ACH;i++) {
        if (achievements[i].unlocked) {
            snprintf(buf,256,"[x] %s",achievements[i].name);
            DrawTextEx(font,buf,(Vector2){x+14,ly},15,1,hex(200,220,200,255)); ly+=20;
            DrawTextEx(font,achievements[i].desc,(Vector2){x+30,ly},13,1,hex(140,180,140,255)); ly+=20;
            shown++;
        }
    }
    if (shown==0) {
        DrawTextEx(font,"No achievements yet. Keep playing!",(Vector2){x+14,ly},15,1,hex(120,120,140,255)); ly+=24;
    }
    ly+=12;
    snprintf(buf,256,"Unlocked: %d / %d",shown,MAX_ACH);
    DrawTextEx(font,buf,(Vector2){x+14,ly},14,1,hex(200,200,100,255));
}

static void drawBottomBar(int w,int h) {
    int by=h-60;
    DrawRectangle(0,by,w,60,hex(16,16,26,255));
    DrawLine(0,by,w,by,hex(45,45,60,255));

    DrawTextEx(font,">",(Vector2){10,by+6},20,1,hex(80,180,255,255));
    DrawTextEx(font,actionDesc,(Vector2){34,by+8},17,1,hex(180,200,230,255));

    // Progress + tier bar
    int bx=12,by2=by+34,bw2=w-24;
    Color tierCol = hex(180,130,255,255);
    bar(bx,by2,bw2-200,14,S.pop/(float)WIN_POP,hpColor(S.pop,WIN_POP),hex(25,25,40,255),hex(55,55,72,255));
    char buf[64]; snprintf(buf,64,"Pop: %d/%d",S.pop,WIN_POP);
    DrawTextEx(font,buf,(Vector2){bx+bw2-190,by2},14,1,hex(200,200,220,255));

    // Tier indicator
    for (int ti=0; ti<=4; ti++) {
        int tix = bx + (bw2-200) * ti / 4;
        Color tc = ti <= currentTier() ? hex(180,130,255,255) : hex(50,50,70,255);
        DrawRectangle(tix, by2-10, 4, 8, tc);
        if (ti==currentTier()) {
            snprintf(buf,64,"%s",tierName(ti));
            DrawTextEx(font,buf,(Vector2){tix-20,by2-28},12,1,tierCol);
        }
    }
}

void uiDraw(int w,int h) {
    drawTopBar(w);
    drawMiniBars(w);
    drawTabs(w);

    int contentY=86, contentH=h-contentY-64;
    int margin=12;

    if(tab==0) drawOverview(margin,contentY,w-margin*2,contentH);
    else if(tab==1) drawBuildings(margin,contentY,w-margin*2,contentH);
    else if(tab==2) drawTech(margin,contentY,w-margin*2,contentH);
    else if(tab==3) drawCivs(margin,contentY,w-margin*2,contentH);
    else if(tab==4) drawAchievements(margin,contentY,w-margin*2,contentH);

    drawBottomBar(w,h);

    // Overlays
    if(gameOver) {
        DrawRectangle(0,0,w,h,hex(20,10,10,200));
        DrawTextEx(font,"GAME OVER",(Vector2){w/2-160,h/2-45},48,2,hex(255,50,50,255));
        DrawTextEx(font,"Press N for new game",(Vector2){w/2-140,h/2+20},24,1,hex(200,200,200,255));
    }
    if(won) {
        DrawRectangle(0,0,w,h,hex(10,20,10,200));
        DrawTextEx(font,"VICTORY",(Vector2){w/2-140,h/2-45},48,2,hex(80,255,80,255));
        DrawTextEx(font,"Your civilization endures!",(Vector2){w/2-180,h/2+20},24,1,hex(200,255,200,255));
        DrawTextEx(font,"Press N for new game",(Vector2){w/2-140,h/2+50},20,1,hex(160,220,160,255));
    }
}
