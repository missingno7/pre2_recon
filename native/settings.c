#include "settings.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

typedef struct Field { const char *key, *choices; size_t offset; double min,max,step; int real,boolean; } Field;
#define I(k,lo,hi,step) {#k,NULL,offsetof(Pre2Settings,k),lo,hi,step,0,0}
#define B(k) {#k,NULL,offsetof(Pre2Settings,k),0,1,1,0,1}
#define E(k,c) {#k,c,offsetof(Pre2Settings,k),0,0,1,0,0}
static const Field fields[SET_COUNT]={
    E(mode,"faithful|enhanced|widescreen"), E(aspect,"16:9|16:10|21:9|window"),
    B(interpolate), B(smooth_camera), B(smooth_transitions),
    {"camera_tau",NULL,offsetof(Pre2Settings,camera_tau),.001,10,.02,1,0},
    E(hud,"anchored|left|centre|right"), E(backdrop,"black|mirror|stretch"),
    I(wide_sim,-1,640,16), B(fullscreen), I(window_scale,1,6,1), B(vsync),
    I(frame_cap,0,1000,1), B(integer_scaling), E(pixel_aspect,"square|crt"),
    I(volume,0,100,5), B(mute), B(performance)
};
static double get(const Pre2Settings *s,int id)
{
    const char *p=(const char *)s+fields[id].offset;
    return fields[id].real?*(const double *)p:*(const int *)p;
}
static void put(Pre2Settings *s,int id,double v)
{
    char *p=(char *)s+fields[id].offset;
    if (fields[id].real) *(double *)p=v; else *(int *)p=(int)v;
}
Pre2Settings settings_defaults(void)
{
    Pre2Settings s={0}; s.camera_tau=.12; s.window_scale=3; s.vsync=1; s.volume=100; return s;
}
const char *settings_key(Pre2Setting id) { return (unsigned)id<SET_COUNT?fields[id].key:""; }
static int choice(const char *list,int index,char *out,size_t n)
{
    if (!list || index<0) return 0;
    while (index-- && list) { list=strchr(list,'|'); if (list) list++; }
    if (!list) return 0;
    size_t len=strcspn(list,"|");
    if (len>=n) return 0;
    memcpy(out,list,len); out[len]=0; return 1;
}
int settings_set(Pre2Settings *s,Pre2Setting id,const char *value)
{
    if ((unsigned)id>=SET_COUNT) return -1;
    const Field *f=&fields[id]; double v; char *end;
    if (f->choices) {
        char name[32]; int i=0;
        while (choice(f->choices,i,name,sizeof name)) { if (!strcmp(name,value)) { put(s,id,i); return 0; } i++; }
        return -1;
    }
    if (id==SET_WIDE_SIM && !strcmp(value,"auto")) v=-1;
    else if (id==SET_WIDE_SIM && !strcmp(value,"off")) v=0;
    else if (f->boolean && (!strcmp(value,"true") || !strcmp(value,"on"))) v=1;
    else if (f->boolean && (!strcmp(value,"false") || !strcmp(value,"off"))) v=0;
    else {
        if (!*value || isspace((unsigned char)*value)) return -1;
        errno=0; v=strtod(value,&end);
        if (*end || errno || !isfinite(v)) return -1;
    }
    if ((id==SET_CAMERA_TAU?!(v>0 && v<=10):(v<f->min || v>f->max)) || (!f->real && v!=floor(v))) return -1;
    if (id==SET_WIDE_SIM && v!=0 && v!=-1 && (v<320 || (int)v%16)) return -1;
    put(s,id,v); return 0;
}
void settings_value(const Pre2Settings *s,Pre2Setting id,char *out,size_t n)
{
    if ((unsigned)id>=SET_COUNT) { if(n) *out=0; return; }
    const Field *f=&fields[id]; double v=get(s,id);
    if (f->choices) { if (!choice(f->choices,(int)v,out,n) && n) *out=0; }
    else if (id==SET_WIDE_SIM && v<=0) snprintf(out,n,"%s",v<0?"auto":"off");
    else if (f->boolean) snprintf(out,n,"%s",v?"true":"false");
    else snprintf(out,n,f->real?"%.6g":"%.0f",v);
}
void settings_copy(Pre2Settings *dst,const Pre2Settings *src,Pre2Setting id) { put(dst,id,get(src,id)); }
void settings_adjust(Pre2Settings *s,Pre2Setting id,int d)
{
    const Field *f=&fields[id]; double v=get(s,id); d=d<0?-1:1;
    if (f->choices) { char b[32]; int count=0; while(choice(f->choices,count,b,sizeof b)) count++; v=((int)v+d+count)%count; }
    else if (id==SET_WIDE_SIM) {
        int index=v==0?0:v==-1?1:2+((int)v-320)/16;
        index=(index+d+23)%23; v=index==0?0:index==1?-1:320+(index-2)*16;
    } else if (id==SET_FRAME_CAP) {
        static const int caps[]={0,30,60,120,144,240}; int index=0;
        while(index<5 && caps[index]<v) index++;
        v=caps[(index+d+6)%6];
    } else { v+=d*f->step; if(v>f->max) v=f->min; if(v<f->min) v=f->max; }
    put(s,id,v);
}
PresenterOptions settings_presenter(const Pre2Settings *s)
{
    PresenterOptions p=presenter_defaults();
    static const double aspects[]={16.0/9,1.6,21.0/9,0};
    static const EnhHud hud[]={ENH_HUD_ANCHORED,ENH_HUD_LEFT,ENH_HUD_CENTRE,ENH_HUD_RIGHT};
    p.mode=(PresenterMode)s->mode; p.aspect=aspects[s->aspect]; p.hud=hud[s->hud]; p.backdrop=(EnhBackdrop)s->backdrop;
    p.interpolate=s->interpolate; p.smooth_camera=s->smooth_camera; p.smooth_transitions=s->smooth_transitions; p.camera_tau=s->camera_tau;
    return p;
}

/* Strict JSON syntax, bounded recursion and strings. Unknown compound values are
 * parsed too, so trailing junk or malformed unknown fields cannot hide errors. */
typedef struct Json { const char *p; int bad; } Json;
static void ws(Json *j) { while(*j->p && strchr(" \r\n\t",*j->p)) j->p++; }
static int take(Json *j,char c) { ws(j); if(*j->p!=c) {j->bad=1; return 0;} j->p++; return 1; }
static void string(Json *j,char *out,size_t cap)
{
    size_t n=0; if(!take(j,'"')) return;
    while(*j->p && *j->p!='"') {
        unsigned char c=(unsigned char)*j->p++;
        if(c<32) {j->bad=1; break;}
        if(c=='\\') {
            c=(unsigned char)*j->p;
            if(!c) {j->bad=1; break;} j->p++;
            if(c=='u') {
                unsigned code=0;
                for(int k=0;k<4;k++) {
                    unsigned char hex=(unsigned char)*j->p;
                    if(!isxdigit(hex)) {j->bad=1; break;}
                    code=code*16+(hex<='9'?hex-'0':(hex|32)-'a'+10); j->p++;
                }
                c=code>0 && code<128?(unsigned char)code:'?';
            }
            else { const char *esc="\"\\/bfnrt"; if(!c || !strchr(esc,c)) {j->bad=1; break;}
                if(c=='b') c='\b'; else if(c=='f') c='\f'; else if(c=='n') c='\n'; else if(c=='r') c='\r'; else if(c=='t') c='\t'; }
        }
        if(out && n+1<cap) out[n]=c;
        n++;
    }
    take(j,'"');
    if(out && cap) out[n<cap?n:cap-1]=0;
    /* Oversized unknown strings remain valid; known names/values cannot match. */
    if(out && n>=cap && cap) out[0]=0;
}
static void value(Json *j,char *out,size_t cap,int depth,int *quoted)
{
    ws(j); if(out && cap) *out=0;
    if(depth>32) {j->bad=1; return;}
    if(*j->p=='"') { if(quoted) *quoted=1; string(j,out,cap); return; }
    if(*j->p=='{' || *j->p=='[') {
        char close=*j->p++=='{'?'}':']'; ws(j);
        if(*j->p==close) {j->p++; return;}
        for(;;) {
            if(close=='}') {string(j,NULL,0); take(j,':');}
            value(j,NULL,0,depth+1,NULL); if(j->bad) return; ws(j);
            if(*j->p==close) {j->p++; return;} if(!take(j,',')) return;
        }
    }
    const char *begin=j->p;
    if(*j->p=='-' || isdigit((unsigned char)*j->p)) {
        if(*j->p=='-') j->p++;
        if(*j->p=='0') j->p++;
        else if(*j->p>='1' && *j->p<='9') {while(isdigit((unsigned char)*j->p)) j->p++;}
        else {j->bad=1; return;}
        if(*j->p=='.') {j->p++; if(!isdigit((unsigned char)*j->p)) j->bad=1; while(isdigit((unsigned char)*j->p)) j->p++;}
        if(*j->p=='e' || *j->p=='E') {j->p++; if(*j->p=='+' || *j->p=='-') j->p++; if(!isdigit((unsigned char)*j->p)) j->bad=1; while(isdigit((unsigned char)*j->p)) j->p++;}
    } else if(!strncmp(j->p,"true",4)) j->p+=4;
    else if(!strncmp(j->p,"false",5)) j->p+=5;
    else if(!strncmp(j->p,"null",4)) j->p+=4;
    else {j->bad=1; return;}
    size_t len=(size_t)(j->p-begin); if(out && len<cap) {memcpy(out,begin,len); out[len]=0;}
}
int settings_parse(Pre2Settings *out,const char *json,FILE *warnings)
{
    Pre2Settings s=settings_defaults(),defaults=s; Json j={json,0};
    *out=s; if(!take(&j,'{')) goto malformed;
    ws(&j); if(*j.p!='}') for(;;) {
        char key[80]={0},v[128]={0}; int quoted=0;
        string(&j,key,sizeof key); take(&j,':'); value(&j,v,sizeof v,0,&quoted);
        if(j.bad) goto malformed;
        if(!strcmp(key,"version")) {
            if(quoted || strcmp(v,"1")) { if(warnings) fprintf(warnings,"settings: unsupported version; using defaults\n"); return -1; }
        } else for(int id=0;id<SET_COUNT;id++) if(!strcmp(key,fields[id].key)) {
            int string_type=fields[id].choices!=NULL || (id==SET_WIDE_SIM && (!strcmp(v,"auto") || !strcmp(v,"off")));
            int bad=quoted!=string_type || (fields[id].boolean && strcmp(v,"true") && strcmp(v,"false"));
            if(bad || settings_set(&s,(Pre2Setting)id,v)) {
                settings_copy(&s,&defaults,(Pre2Setting)id);
                if(warnings) fprintf(warnings,"settings: invalid %s; using default\n",key);
            }
        }
        ws(&j); if(*j.p=='}') break; if(!take(&j,',')) goto malformed;
    }
    take(&j,'}'); ws(&j); if(j.bad || *j.p) goto malformed;
    *out=s; return 0;
malformed:
    if(warnings) fprintf(warnings,"settings: malformed JSON; using defaults\n");
    return -1;
}
int settings_load(Pre2Settings *out,const char *path,FILE *warnings)
{
    *out=settings_defaults(); FILE *f=fopen(path,"rb");
    if(!f) {if(errno==ENOENT) return 1; if(warnings) fprintf(warnings,"settings: cannot read %s\n",path); return -1;}
    char *buf=malloc(65538); if(!buf) {fclose(f); return -1;}
    size_t n=fread(buf,1,65537,f); int bad=ferror(f) || n>65536; fclose(f); buf[n]=0;
    int embedded=memchr(buf,0,n)!=NULL;
    int result=bad || embedded?-1:settings_parse(out,buf,warnings);
    if((bad || embedded) && warnings) fprintf(warnings,"settings: oversized, unreadable or embedded-NUL file; using defaults\n");
    free(buf); return result;
}
int settings_save(const Pre2Settings *s,const char *path,FILE *warnings)
{
    size_t len=strlen(path); char *tmp=malloc(len+5); if(!tmp) return -1;
    memcpy(tmp,path,len); memcpy(tmp+len,".tmp",5); FILE *f=fopen(tmp,"wb"); int bad=!f;
    if(f) {
        if(fprintf(f,"{\n  \"version\": 1")<0) bad=1;
        for(int id=0;id<SET_COUNT;id++) {
            char v[64]; settings_value(s,(Pre2Setting)id,v,sizeof v);
            if(fields[id].real) snprintf(v,sizeof v,"%.17g",get(s,id));
            int quote=fields[id].choices!=NULL || (id==SET_WIDE_SIM && get(s,id)<=0);
            if(fprintf(f,",\n  \"%s\": %s%s%s",fields[id].key,quote?"\"":"",v,quote?"\"":"")<0) bad=1;
        }
        if(fputs("\n}\n",f)==EOF) bad=1;
        if(fflush(f)) bad=1;
        if(fclose(f)) bad=1;
    }
    if(!bad) {
#ifdef _WIN32
        /* C rename cannot replace an existing destination on Windows. This is
         * the same-directory atomic replacement, never delete-then-rename. */
        bad=!MoveFileExA(tmp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
#else
        bad=rename(tmp,path)!=0;
#endif
    }
    if(bad) {remove(tmp); if(warnings) fprintf(warnings,"settings: cannot save %s\n",path);}
    free(tmp); return bad?-1:0;
}
