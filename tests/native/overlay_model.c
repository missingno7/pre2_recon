#include "native/overlay.h"
#include <assert.h>
#undef assert
#define assert(c) do { if(!(c)) {fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c); exit(1);} } while(0)

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

static void bmp(const char *path,const uint32_t *px,int w,int h)
{
    FILE *f=fopen(path,"wb"); assert(f); uint8_t hd[54]={'B','M'};
    uint32_t size=54+(uint32_t)w*h*4; int32_t hh=-h;
    memcpy(hd+2,&size,4); hd[10]=54; hd[14]=40; memcpy(hd+18,&w,4); memcpy(hd+22,&hh,4); hd[26]=1; hd[28]=32;
    assert(fwrite(hd,1,54,f)==54); assert(fwrite(px,4,(size_t)w*h,f)==(size_t)w*h); assert(!fclose(f));
}
static uint64_t hash(const uint32_t *px,size_t n)
{
    uint64_t h=1469598103934665603ull;
    for(size_t i=0;i<n;i++) {h^=px[i]; h*=1099511628211ull;}
    return h;
}
int main(void)
{
    Pre2Overlay o={0}; Pre2Settings s=settings_defaults(); unsigned seen[SET_COUNT]={0};
    overlay_key(&o,&s,OVERLAY_RIGHT); assert(!o.open && !s.mode);
    overlay_key(&o,&s,OVERLAY_TOGGLE); assert(o.open);
    for(int tab=0;tab<4;tab++) {
        assert(o.tab==tab && o.row==0);
        int editable=0;
        for(int row=0;row<overlay_rows(tab);row++) {
            Pre2Setting id=overlay_item(tab,row); if(id==SET_COUNT) continue;
            assert(o.row==row); seen[id]++; editable++;
            for(int d=0;d<3;d++) {
                Pre2Settings before=s; OverlayKey key=d==0?OVERLAY_RIGHT:d==1?OVERLAY_LEFT:OVERLAY_ENTER;
                assert(overlay_key(&o,&s,key)==id);
                for(int other=0;other<SET_COUNT;other++) {
                    char a[80],b[80]; settings_value(&s,(Pre2Setting)other,a,sizeof a); settings_value(&before,(Pre2Setting)other,b,sizeof b);
                    assert((strcmp(a,b)!=0)==(other==(int)id));
                }
            }
            overlay_key(&o,&s,OVERLAY_DOWN);
        }
        assert(editable && !o.row);
        overlay_key(&o,&s,OVERLAY_UP); assert(overlay_item(tab,o.row)!=SET_COUNT);
        overlay_key(&o,&s,OVERLAY_DOWN); assert(!o.row);
        overlay_key(&o,&s,OVERLAY_TAB);
    }
    for(int id=0;id<SET_COUNT;id++) assert(seen[id]==1);
    overlay_key(&o,&s,OVERLAY_BACKTAB); assert(o.tab==3);
    overlay_key(&o,&s,OVERLAY_ESCAPE); assert(!o.open);
    overlay_key(&o,&s,OVERLAY_TOGGLE); assert(o.open);
    mkdir("build/overlay_shots");
    const int sizes[][2]={{320,200},{640,400},{960,600},{1280,720},{1920,1080},{257,151}};
    const uint64_t expected[]={0xead8751dc9879bb7ull,0xa520e3807df8b5d7ull,0x282887caf80d1f3dull,
        0x2cc166cd5d00f4baull,0x58d0ddf1800480cfull,0xca993498d4004dc9ull};
    unsigned mismatched=0;   /* every shot is written before the golden check */
    for(unsigned i=0;i<sizeof sizes/sizeof *sizes;i++) {
        int w=sizes[i][0],h=sizes[i][1]; size_t n=(size_t)w*h;
        uint32_t *a=malloc((n+2)*4),*b=malloc((n+2)*4); assert(a && b);
        a[0]=b[0]=0x12345678; a[n+1]=b[n+1]=0xabcdef01;
        for(size_t p=1;p<=n;p++) a[p]=b[p]=((p%w)*255/w)<<16 | ((p/w)*255/h)<<8 | 0x42;
        o.tab=(int)(i%4); o.row=0;
        OverlayPerf perf={144,72.8,-.4,1};
        overlay_render(&o,&s,&perf,a+1,w,h); overlay_render(&o,&s,&perf,b+1,w,h);
        assert(!memcmp(a,b,(n+2)*4)); assert(a[0]==0x12345678 && a[n+1]==0xabcdef01);
        char path[128]; snprintf(path,sizeof path,"build/overlay_shots/overlay_%dx%d.bmp",w,h); bmp(path,a+1,w,h);
        printf("overlay %dx%d hash %016llx\n",w,h,(unsigned long long)hash(a+1,n));
        if(hash(a+1,n)!=expected[i]) mismatched++;
        free(a); free(b);
    }
    assert(!mismatched);
    puts("overlay_model: every item changes exactly one setting; navigation, clipping and deterministic rendering pass");
    return 0;
}
