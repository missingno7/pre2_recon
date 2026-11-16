#include "overlay.h"
#include <string.h>
#include <stdio.h>

typedef struct Item { const char *label; Pre2Setting id; } Item;
static const Item display[]={
    {"MODE",SET_MODE},{"ASPECT (WIDE)",SET_ASPECT},{"FULLSCREEN",SET_FULLSCREEN},
    {"WINDOW SCALE",SET_WINDOW_SCALE},{"VSYNC",SET_VSYNC},{"FRAME CAP",SET_FRAME_CAP},
    {"INTEGER SCALING",SET_INTEGER_SCALING},{"PIXEL ASPECT",SET_PIXEL_ASPECT},
    {"PERFORMANCE",SET_PERFORMANCE},{"0 CAP = UNLIMITED; VSYNC MAY LIMIT",SET_COUNT}
};
static const Item enhancements[]={
    {"INTERPOLATION",SET_INTERPOLATE},{"SMOOTH CAMERA",SET_SMOOTH_CAMERA},
    {"CAMERA TAU (SEC)",SET_CAMERA_TAU},{"SMOOTH TRANSITIONS",SET_SMOOTH_TRANSITIONS},
    {"HUD",SET_HUD},{"BACKDROP",SET_BACKDROP},
    {"LARGER TAU = SOFTER CAMERA",SET_COUNT},{"SMOOTH CAMERA IMPLIES INTERPOLATION",SET_COUNT}
};
static const Item audio[]={
    {"MASTER VOLUME %",SET_VOLUME},{"MUTE",SET_MUTE},
    {"OUTPUT GAIN ONLY; ORIGINAL MIX UNCHANGED",SET_COUNT}
};
static const Item experimental[]={
    {"TRUE WIDESCREEN",SET_WIDE_SIM},{"CHANGES GAMEPLAY",SET_COUNT},
    {"APPLIES ON NEXT BOOT - RESTART HOST",SET_COUNT},
    {"DEMOS ALWAYS USE ORIGINAL GEOMETRY",SET_COUNT},
    {"AUTO USES THE SAVED / CLI ASPECT",SET_COUNT}
};
static const Item *const tabs[]={display,enhancements,audio,experimental};
static const int counts[]={sizeof display/sizeof *display,sizeof enhancements/sizeof *enhancements,
    sizeof audio/sizeof *audio,sizeof experimental/sizeof *experimental};
static const char *const names[]={"DISPLAY","ENHANCEMENTS","AUDIO","EXPERIMENTAL"};
int overlay_rows(int tab) {return tab>=0 && tab<4?counts[tab]:0;}
Pre2Setting overlay_item(int tab,int row)
{return tab>=0 && tab<4 && row>=0 && row<counts[tab]?tabs[tab][row].id:SET_COUNT;}
Pre2Setting overlay_key(Pre2Overlay *o,Pre2Settings *s,OverlayKey key)
{
    if(key==OVERLAY_TOGGLE) {o->open=!o->open; return SET_COUNT;}
    if(!o->open) return SET_COUNT;
    if(key==OVERLAY_ESCAPE) {o->open=0; return SET_COUNT;}
    if(key==OVERLAY_TAB || key==OVERLAY_BACKTAB) {o->tab=(o->tab+(key==OVERLAY_TAB?1:3))%4; o->row=0;}
    else if(key==OVERLAY_UP || key==OVERLAY_DOWN) {
        int d=key==OVERLAY_UP?-1:1;
        do {o->row=(o->row+d+counts[o->tab])%counts[o->tab];} while(overlay_item(o->tab,o->row)==SET_COUNT);
    } else if(key==OVERLAY_LEFT || key==OVERLAY_RIGHT || key==OVERLAY_ENTER) {
        Pre2Setting id=overlay_item(o->tab,o->row);
        if(id!=SET_COUNT) {settings_adjust(s,id,key==OVERLAY_LEFT?-1:1); return id;}
    }
    return SET_COUNT;
}

/* Authored 5x7 face (rows of five bits, leftmost pixel in bit 4); six-pixel advance.
 * A 3x5 face cannot tell M, N and H apart. No external font assets. */
#define GLYPH_W 6
typedef struct Glyph { char c; unsigned char row[7]; } Glyph;
static const Glyph font[]={
 {'A',{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},{'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},{'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},{'D',{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
 {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},{'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},{'G',{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}},{'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
 {'I',{0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},{'J',{0x07,0x02,0x02,0x02,0x02,0x12,0x0C}},{'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}},{'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
 {'M',{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},{'N',{0x11,0x11,0x19,0x15,0x13,0x11,0x11}},{'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},{'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
 {'Q',{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},{'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},{'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},{'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
 {'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},{'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},{'W',{0x11,0x11,0x11,0x15,0x15,0x15,0x0A}},{'X',{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
 {'Y',{0x11,0x11,0x11,0x0A,0x04,0x04,0x04}},{'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},{'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},{'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
 {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},{'3',{0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}},{'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},{'5',{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
 {'6',{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},{'7',{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},{'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},{'9',{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
 {'.',{0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}},{':',{0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00}},{'/',{0x00,0x01,0x02,0x04,0x08,0x10,0x00}},{'-',{0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
 {'+',{0x00,0x04,0x04,0x1F,0x04,0x04,0x00}},{'(',{0x02,0x04,0x08,0x08,0x08,0x04,0x02}},{')',{0x08,0x04,0x02,0x02,0x02,0x04,0x08}},{'%',{0x18,0x19,0x02,0x04,0x08,0x13,0x03}},
 {'=',{0x00,0x00,0x1F,0x00,0x1F,0x00,0x00}},{'>',{0x08,0x04,0x02,0x01,0x02,0x04,0x08}},{'<',{0x02,0x04,0x08,0x10,0x08,0x04,0x02}},{',',{0x00,0x00,0x00,0x00,0x0C,0x04,0x08}},
 {'\'',{0x0C,0x04,0x08,0x00,0x00,0x00,0x00}},{'?',{0x0E,0x11,0x01,0x02,0x04,0x00,0x04}},{'!',{0x04,0x04,0x04,0x04,0x04,0x00,0x04}},{'*',{0x00,0x04,0x15,0x0E,0x15,0x04,0x00}}
};
static void rect(uint32_t *px,int w,int h,int x,int y,int rw,int rh,uint32_t col)
{
    int x1=x+rw,y1=y+rh; if(x<0)x=0; if(y<0)y=0; if(x1>w)x1=w; if(y1>h)y1=h;
    for(int j=y;j<y1;j++) for(int i=x;i<x1;i++) px[(size_t)j*w+i]=col;
}
static void text(uint32_t *px,int w,int h,int x,int y,int scale,const char *str,uint32_t col)
{
    for(;*str;str++,x+=GLYPH_W*scale) {
        char c=*str; if(c>='a' && c<='z') c-='a'-'A';
        for(size_t k=0;k<sizeof font/sizeof *font;k++) if(font[k].c==c) {
            for(int j=0;j<7;j++) for(int i=0;i<5;i++) if(font[k].row[j] & (16>>i))
                rect(px,w,h,x+i*scale,y+j*scale,scale,scale,col);
            break;
        }
    }
}
static int scale_for(int w,int h) {int s=w/320,t=h/200; if(t<s)s=t; return s>0?s:1;}
int overlay_scale(int w,int h) {return scale_for(w,h);}
int overlay_performance_rows(int scale) {return 11*scale;}
void overlay_performance(const OverlayPerf *p,uint32_t *px,int w,int h)
{
    overlay_performance_strip(p,px,w,h,scale_for(w,h));
}
void overlay_performance_strip(const OverlayPerf *p,uint32_t *px,int w,int h,int s)
{
    if(!p || !px || w<=0 || h<=0 || s<=0) return;
    char b[100];
    snprintf(b,sizeof b,"FPS %.1f TPS %.1f DRIFT %+.1f MS DRAW %.1f SHOW %.1f%s",p->fps,p->tps,p->drift_ms,p->draw_ms,p->show_ms,p->paused?" PAUSED":"");
    rect(px,w,h,0,0,(int)strlen(b)*GLYPH_W*s+6*s,11*s,0x0d1822);
    text(px,w,h,3*s,2*s,s,b,0xb8e8c4);
}
void overlay_render(const Pre2Overlay *o,const Pre2Settings *s,const OverlayPerf *perf,uint32_t *px,int w,int h)
{
    if(!o->open || !px || w<=0 || h<=0) return;
    for(size_t i=0;i<(size_t)w*h;i++) px[i]=(px[i]>>2)&0x3f3f3f;
    int z=scale_for(w,h),x=(w-304*z)/2,y=(h-180*z)/2;
    rect(px,w,h,x,y,304*z,180*z,0x7296ad);
    rect(px,w,h,x+z,y+z,302*z,178*z,0x111c29);
    text(px,w,h,x+9*z,y+9*z,z,"PREHISTORIK 2 / SETTINGS / PAUSED",0xf0ead6);
    int tx=x+8*z;
    for(int t=0;t<4;t++) {
        int len=(int)strlen(names[t])*GLYPH_W+7;
        rect(px,w,h,tx,y+23*z,len*z,12*z,t==o->tab?0x3a6983:0x203449);
        text(px,w,h,tx+4*z,y+27*z,z,names[t],0xe3edf2); tx+=(len+3)*z;
    }
    for(int r=0;r<counts[o->tab];r++) {
        const Item *item=&tabs[o->tab][r]; int yy=y+(43+r*10)*z;
        if(r==o->row) rect(px,w,h,x+6*z,yy-2*z,292*z,10*z,0x294b65);
        text(px,w,h,x+10*z,yy,z,item->label,item->id==SET_COUNT?0x9aadb9:0xe4edf1);
        if(item->id!=SET_COUNT) {
            char v[64]; settings_value(s,item->id,v,sizeof v);
            text(px,w,h,x+198*z,yy,z,v,0x98dfce);
        }
    }
    text(px,w,h,x+10*z,y+151*z,z,"UP/DOWN SELECT   LEFT/RIGHT CHANGE",0xbbd1df);
    text(px,w,h,x+10*z,y+161*z,z,"TAB NEXT   ENTER CHANGE   ESC CLOSE",0xbbd1df);
    if(perf && s->performance) overlay_performance(perf,px,w,h);
}
