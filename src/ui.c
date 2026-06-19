#include "game.h"
#include <stdio.h>
#include <string.h>

static Font font;

static Color col(int r,int g,int b,int a) { return (Color){r,g,b,a}; }
static Color hpCol(int v,int hi) {
    float p=v/(float)hi;
    return p>0.6f?col(80,220,80,255):p>0.3f?col(220,200,60,255):col(220,60,60,255);
}

static int tw(const char *t, int sz) {
    return (int)MeasureTextEx(font,t,sz,1).x;
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
    DrawRectangle(x,y,w,h,col(16,16,24,255));
    DrawRectangleLines(x,y,w,h,col(40,40,55,255));
    if(title) {
        DrawRectangle(x+1,y+1,w-2,26,col(22,22,34,255));
        DrawTextEx(font,title,(Vector2){x+10,y+4},16,1,titleCol);
    }
}

static void drawTopBar(int w) {
    // Background
    DrawRectangle(0,0,w,52,col(20,20,32,255));
    DrawLine(0,52,w,52,col(45,45,60,255));

    char buf[256];

    // Row 1: Title + compact resources + pop/season (y=5)
    DrawTextEx(font,"EMERGENCE",(Vector2){12,5},20,1,col(230,230,100,255));
    int tx=12+tw("EMERGENCE",20)+16;

    snprintf(buf,256,"F:%d W:%d S:%d G:%d H:%d",S.food,S.wood,S.stone,S.gold,S.herbs);
    DrawTextEx(font,buf,(Vector2){tx,9},15,1,col(180,180,210,255));

    // Pop/season right-aligned
    Color sc=S.season==3?col(180,220,255,255):S.season==0?col(100,220,100,255):col(200,200,200,255);
    snprintf(buf,256,"P:%d/%d  %s  D%d T%d",S.pop,S.popCap,SEASONS[S.season],S.day,S.turn);
    int pw=tw(buf,15);
    DrawTextEx(font,buf,(Vector2){w-pw-12,9},15,1,sc);

    // Row 2: Mini bars along the bottom of top bar (y=30-50)
    int bw2=(w-60)/4; // 4 bars with gaps
    int by=33;
    int bh=14;
    int bxs[4]={12, 12+bw2+12, 12+2*(bw2+12), 12+3*(bw2+12)};
    struct{const char*l;int v;int m;Color c;}bars[4]={
        {"Mor",S.morale,100,col(80,220,80,255)},
        {"Hlth",S.health,100,col(80,200,220,255)},
        {"Know",(int)S.knowledge,100,col(180,130,220,255)},
        {"Faith",(int)S.faith,100,col(255,200,100,255)},
    };
    for(int i=0;i<4;i++){
        int bx=bxs[i];
        Color fc=bars[i].c;
        float pct=clamp01(bars[i].v/(float)bars[i].m);
        bar(bx,by,bw2,bh,pct,fc,col(25,25,40,255),col(50,50,65,255));
        DrawTextEx(font,bars[i].l,(Vector2){bx+2,by-1},11,1,col(120,120,150,255));
        char vb[16]; snprintf(vb,16,"%d",bars[i].v);
        DrawTextEx(font,vb,(Vector2){bx+bw2-tw(vb,11)-2,by+bh-12},11,1,col(200,200,220,255));
    }
}

static void drawTabs(int w) {
    int ty=54;
    DrawRectangle(0,ty,w,28,col(14,14,22,255));
    DrawLine(0,ty+28,w,ty+28,col(40,40,55,255));
    const char *tabs[]={"Q:OVERVIEW","W:BUILD","E:TECH","R:CIVS","A:ACHIEVE"};
    int n=5, tabW=w/n;
    for(int i=0;i<n;i++) {
        int x=i*tabW;
        if(i==tab) {
            DrawRectangle(x,ty,tabW,28,col(25,25,38,255));
            DrawLine(x,ty+28,x+tabW,ty+28,col(220,220,100,255));
        }
        Color tc=i==tab?col(230,230,150,255):col(120,120,150,255);
        int tw2=tw(tabs[i],15);
        DrawTextEx(font,tabs[i],(Vector2){x+(tabW-tw2)/2,ty+6},15,1,tc);
    }
}

static void drawOverview(int x,int y,int w,int h) {
    int gap=10;
    int colW=(w-gap*2)/3;
    int colH=h-4;

    // LEFT: Resources & Tier
    panel(x,y,colW,colH,"RESOURCES",col(160,200,255,255));
    int ly=y+32;
    char buf[256];

    struct{const char*n;int v;}rs[]={
        {"Food",S.food},{"Wood",S.wood},{"Stone",S.stone},{"Gold",S.gold},
        {"Herbs",S.herbs},{"Tools",S.tools},{"Knowledge",S.knowledge},{"Faith",S.faith},
    };
    for(int i=0;i<8;i++) {
        Color rc=col(200,200,220,255);
        if(i==0)rc=S.food>=S.pop*3?col(100,220,100,255):col(220,180,100,255);
        snprintf(buf,128,"%8s: %d",rs[i].n,rs[i].v);
        DrawTextEx(font,buf,(Vector2){x+12,ly},14,1,rc); ly+=20;
    }
    ly+=2;
    int eaten=S.pop*2 - S.buildings[13]; if(eaten<0)eaten=0;
    snprintf(buf,256,"Consume: %d food/turn%s",eaten,S.season==3?" (x2)":"");
    Color cc=S.food>=eaten?col(140,200,140,255):col(220,120,120,255);
    DrawTextEx(font,buf,(Vector2){x+12,ly},14,1,cc); ly+=19;

    snprintf(buf,256,"Soldiers: %d",S.soldiers);
    DrawTextEx(font,buf,(Vector2){x+12,ly},14,1,col(180,180,200,255)); ly+=19;

    // Tier
    int t=currentTier();
    snprintf(buf,256,"Tier: %s",tierName(t));
    DrawTextEx(font,buf,(Vector2){x+12,ly},14,1,col(180,130,255,255)); ly+=18;
    snprintf(buf,256,"Colonies: %d  Ships: %d",S.colonyCount,S.shipsBuilt);
    DrawTextEx(font,buf,(Vector2){x+12,ly},13,1,col(140,180,220,255)); ly+=17;
    if(S.galaxiesReached>0){
        snprintf(buf,256,"Galaxies: %d",S.galaxiesReached);
        DrawTextEx(font,buf,(Vector2){x+12,ly},13,1,col(160,200,255,255));ly+=17;
    }
    if(S.universesReached>0){
        snprintf(buf,256,"Universes: %d",S.universesReached);
        DrawTextEx(font,buf,(Vector2){x+12,ly},13,1,col(200,150,255,255));ly+=17;
    }

    // Ongoing
    const char *ongoing[8]; int oc=0;
    if(S.hunting>0)ongoing[oc++]=">> Hunting";
    if(S.farming>0)ongoing[oc++]=">> Farming";
    if(S.building>0)ongoing[oc++]=">> Building";
    if(S.scouting>0)ongoing[oc++]=">> Scouting";
    if(S.researching>0)ongoing[oc++]=">> Researching";
    if(S.colonizing>0)ongoing[oc++]=">> Colonizing";
    if(oc>0){
        ly+=3; DrawTextEx(font,"--- Ongoing ---",(Vector2){x+12,ly},12,1,col(140,140,170,255)); ly+=17;
        Color ocols[]={col(180,255,100,255),col(100,220,100,255),col(180,180,100,255),
                       col(180,200,255,255),col(200,150,255,255),col(100,200,255,255)};
        for(int i=0;i<oc&&i<6;i++){
            DrawTextEx(font,ongoing[i],(Vector2){x+16,ly},13,1,ocols[i]); ly+=17;
        }
    }

    // CENTER: Population, Jobs, Crop/Hunt, Actions
    int cx=x+colW+gap;
    panel(cx,y,colW,colH,"POPULATION",col(160,200,255,255));
    int jy=y+32;
    int idle=S.pop-S.soldiers-totalAssigned(); if(idle<0)idle=0;
    snprintf(buf,256,"Pop: %d/%d  Idle: %d  Soldiers: %d",S.pop,S.popCap,idle,S.soldiers);
    DrawTextEx(font,buf,(Vector2){cx+12,jy},14,1,idle>0?col(160,220,160,255):col(220,160,160,255)); jy+=21;

    struct{const char*n;int*v;}jobs[]={
        {"Farmer [1]",&S.farmers},{"Woodcutter [2]",&S.woodcutters},{"Miner [3]",&S.miners},
        {"Hunter [4]",&S.hunters},{"Scholar [5]",&S.scholars},{"Priest [6]",&S.priests},
    };
    for(int i=0;i<6;i++) {
        snprintf(buf,128,"  %s: %d",jobs[i].n,*jobs[i].v);
        Color jc=totalAssigned()<S.pop-S.soldiers?col(200,200,220,255):col(100,100,120,255);
        DrawTextEx(font,buf,(Vector2){cx+12,jy},14,1,jc); jy+=18;
    }
    jy+=3;

    // Crop & Hunt zone display
    int cc2=S.selectedCrop;
    snprintf(buf,256,"Crop [J]: %s (C:%d D:%d Y:%d-%d)",
        CROP_NAMES[cc2], CROP_COST[cc2], CROP_DAYS[cc2],
        CROP_MIN_Y[cc2], CROP_MAX_Y[cc2]);
    DrawTextEx(font,buf,(Vector2){cx+12,jy},13,1,col(100,220,100,255)); jy+=17;
    int hz=S.selectedHuntZone;
    snprintf(buf,256,"Hunt [D]: %s (%s D:%d Y:%d-%d)",
        HUNT_NAMES[hz], HUNT_DESC[hz], HUNT_DANGER[hz],
        HUNT_MIN_F[hz], HUNT_MAX_F[hz]);
    DrawTextEx(font,buf,(Vector2){cx+12,jy},13,1,col(180,255,100,255)); jy+=17;

    // Actions
    jy+=2; DrawTextEx(font,"-- Actions --",(Vector2){cx+12,jy},13,1,col(140,140,170,255)); jy+=17;
    const char*acts[]={
        "H=Hunt  F=Farm  G=Gift  B=Home",
        "U=Research  T=Trade  Z=Rest",
        "C=Heal  M=Recruit  S=Scout",
        "P=Train  I=Colony  Y=Ship",
        "O=Explore  K=Save  L=Load  N=New",
    };
    for(int i=0;i<5;i++){DrawTextEx(font,acts[i],(Vector2){cx+12,jy},13,1,col(120,120,150,255));jy+=16;}

    // RIGHT: Journal
    int jx=cx+colW+gap;
    int jw=(x+w)-jx;
    panel(jx,y,jw-4,colH,"JOURNAL",col(160,200,255,255));
    int ly2=y+32, maxVis=(colH-20)/16, start=logN>maxVis?logN-maxVis:0;
    for(int i=start;i<logN;i++){
        DrawTextEx(font,logs[i].msg,(Vector2){jx+10,ly2},14,1,logs[i].col);ly2+=16;
    }
}

static void drawBuildings(int x,int y,int w,int h) {
    panel(x,y,w-4,h,"BUILDINGS  [0-9 build]",col(160,200,255,255));
    int ly=y+32;
    char buf[256];
    for(int i=0;i<MAX_BUILD;i++) {
        Color avail=canBuild(i)?col(200,220,240,255):col(120,120,140,255);
        Color ct=S.buildings[i]>0?col(100,200,255,255):avail;
        snprintf(buf,256,"%d: %-12s [%2d] %s",i,buildingName(i),S.buildings[i],buildingEffect(i));
        DrawTextEx(font,buf,(Vector2){x+10,ly},14,1,ct);
        // Cost
        char cost[48]; cost[0]=0; int pos2=0;
        int vals[4]={buildingCost(i,0),buildingCost(i,1),buildingCost(i,2),buildingCost(i,3)};
        const char *lbl[4]={"F","W","S","G"};
        for(int j=0;j<4;j++)if(vals[j]>0){char tmp[12];snprintf(tmp,12," %s%d",lbl[j],vals[j]);strcpy(cost+pos2,tmp);pos2+=strlen(tmp);}
        int nameEnd=tw(buf,14);
        Color costCol=S.buildings[i]>0?col(80,180,80,255):col(140,140,160,255);
        DrawTextEx(font,cost,(Vector2){x+12+nameEnd+4,ly},13,1,costCol);
        ly+=20;
    }
}

static void drawTech(int x,int y,int w,int h) {
    panel(x,y,w-4,h,"TECHNOLOGY  [0-9 research]",col(160,200,255,255));
    int ly=y+32;
    char buf[256];
    for(int i=0;i<MAX_TECH;i++) {
        Color tc=S.tech[i]?col(60,180,60,255):(S.knowledge>=techCost(i)?col(200,220,240,255):col(120,120,140,255));
        const char*done=S.tech[i]?" [DONE]":"";
        snprintf(buf,256,"%d: %-13s (%3d) %s%s",i,techName(i),techCost(i),techDesc(i),done);
        DrawTextEx(font,buf,(Vector2){x+10,ly},14,1,tc);
        ly+=20;
    }
}

static void drawCivs(int x,int y,int w,int h) {
    panel(x,y,w-4,h,"CIVILIZATIONS",col(160,200,255,255));
    int ly=y+32;
    char buf[256];
    for(int i=0;i<MAX_CIVS;i++) {
        CivData *c=&civs[i];
        Color dc=c->discovered?col(200,200,220,255):col(100,100,120,255);
        const char *s=c->alive?"Alive":"Fallen";
        const char *hd=c->discovered?"":" [Hidden]";
        snprintf(buf,256,"%d: %-10s %s%s  R:%d P:%d T%d",i,c->name,s,hd,c->relation,c->s.pop,c->tier);
        Color tc=(i==selectedCivIdx)?col(255,220,100,255):dc;
        if(i==currentCiv) tc=col(100,200,255,255);
        DrawTextEx(font,buf,(Vector2){x+12,ly},14,1,tc); ly+=21;
    }
    ly+=6;
    DrawTextEx(font,"-- Selected --",(Vector2){x+12,ly},13,1,col(140,140,170,255)); ly+=18;
    if(civs[selectedCivIdx].alive && civs[selectedCivIdx].discovered) {
        CivData *c=&civs[selectedCivIdx];
        snprintf(buf,256,"%s: P:%d F:%d W:%d S:%d G:%d",
            c->name,c->s.pop,c->s.food,c->s.wood,c->s.stone,c->s.gold);
        DrawTextEx(font,buf,(Vector2){x+12,ly},13,1,col(180,200,230,255)); ly+=17;
        snprintf(buf,256,"Soldiers:%d Morale:%d Health:%d Rel:%d",
            c->s.soldiers,c->s.morale,c->s.health,c->relation);
        DrawTextEx(font,buf,(Vector2){x+12,ly},13,1,col(180,200,230,255)); ly+=17;
        if(selectedCivIdx!=currentCiv) {
            DrawTextEx(font,"T=Trade  W=War  V=Switch  A=Ally",(Vector2){x+12,ly},13,1,col(100,180,255,255));
        }
    } else {
        DrawTextEx(font,"Not yet discovered.",(Vector2){x+12,ly},13,1,col(120,120,140,255));
    }
    ly+=20;
    DrawTextEx(font,"[0-5] select  [T]rade [W]ar [A]lly [V]iew",(Vector2){x+12,ly},12,1,col(100,100,130,255));
}

static void drawAchievements(int x,int y,int w,int h) {
    panel(x,y,w-4,h,"ACHIEVEMENTS",col(160,200,255,255));
    int ly=y+32;
    char buf[256];
    int shown=0;
    for(int i=0;i<MAX_ACH;i++) {
        if (achievements[i].unlocked) {
            snprintf(buf,256,"[x] %s",achievements[i].name);
            DrawTextEx(font,buf,(Vector2){x+12,ly},14,1,col(200,220,200,255)); ly+=19;
            DrawTextEx(font,achievements[i].desc,(Vector2){x+26,ly},12,1,col(140,180,140,255)); ly+=17;
            shown++;
        }
    }
    if (shown==0) {
        DrawTextEx(font,"No achievements yet.",(Vector2){x+12,ly},14,1,col(120,120,140,255)); ly+=20;
    }
    ly+=8;
    snprintf(buf,256,"Unlocked: %d / %d",shown,MAX_ACH);
    DrawTextEx(font,buf,(Vector2){x+12,ly},13,1,col(200,200,100,255));
}

static void drawBottomBar(int w,int h) {
    int by=h-56;
    DrawRectangle(0,by,w,56,col(16,16,26,255));
    DrawLine(0,by,w,by,col(45,45,60,255));

    // Action description
    DrawTextEx(font,">",(Vector2){10,by+4},18,1,col(80,180,255,255));
    DrawTextEx(font,actionDesc,(Vector2){30,by+6},15,1,col(180,200,230,255));

    // Progress bar + tier label
    int bx=10, by2=by+30, bw2=w-20;
    bar(bx,by2,bw2-160,14,S.pop/(float)WIN_POP,hpCol(S.pop,WIN_POP),col(25,25,40,255),col(55,55,72,255));
    char buf[64];
    snprintf(buf,64,"%s  %d/%d",tierName(currentTier()),S.pop,WIN_POP);
    DrawTextEx(font,buf,(Vector2){bx+bw2-156,by2},13,1,col(200,200,220,255));
}

void uiDraw(int w,int h) {
    drawTopBar(w);
    drawTabs(w);

    int contentY=86, contentH=h-contentY-60;

    if(tab==0) drawOverview(10,contentY,w-20,contentH);
    else if(tab==1) drawBuildings(10,contentY,w-20,contentH);
    else if(tab==2) drawTech(10,contentY,w-20,contentH);
    else if(tab==3) drawCivs(10,contentY,w-20,contentH);
    else if(tab==4) drawAchievements(10,contentY,w-20,contentH);

    drawBottomBar(w,h);

    if(gameOver) {
        DrawRectangle(0,0,w,h,col(20,10,10,200));
        DrawTextEx(font,"GAME OVER",(Vector2){w/2-160,h/2-45},48,2,col(255,50,50,255));
        DrawTextEx(font,"Press N for new game",(Vector2){w/2-140,h/2+20},24,1,col(200,200,200,255));
    }
    if(won) {
        DrawRectangle(0,0,w,h,col(10,20,10,200));
        DrawTextEx(font,"VICTORY",(Vector2){w/2-140,h/2-45},48,2,col(80,255,80,255));
        DrawTextEx(font,"Your civilization endures!",(Vector2){w/2-180,h/2+20},24,1,col(200,255,200,255));
        DrawTextEx(font,"Press N for new game",(Vector2){w/2-140,h/2+50},20,1,col(160,220,160,255));
    }
}
