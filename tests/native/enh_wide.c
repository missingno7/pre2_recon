/* Bounded synthetic world: no game execution. Check edges, arbitrary camera,
 * world limits, foreground in the margin, HUD placement and sprite overrides. */
#include "native/enh_render.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static EnhLevel level;
static EnhCel cels[1024];
static EnhMap map;
static FrameGameView view;
static uint8_t blocks[65536], palette[256][3];
static void run(int w, int h, EnhOptions *options, double cx, double cy)
{
    size_t stride = (size_t)w + 9, size = stride * (size_t)h;
    uint8_t *p = malloc(size + 128), *saved = malloc(size); assert(p && saved);
    memset(p, 0x7e, size + 128);
    EnhImage im = {0}; im.index = p + 64; im.stride = stride; im.capacity = size;
    uint64_t before = enh_view_hash(&view, 123);
    assert(!enh_render(&view, &level, palette, 15, cx, cy, w, h, options, &im));
    memcpy(saved, im.index, size);
    assert(!enh_render(&view, &level, palette, 15, cx, cy, w, h, options, &im));
    assert(!memcmp(saved, im.index, size)); assert(before == enh_view_hash(&view, 123));
    assert(!memcmp(im.palette, palette, sizeof palette) && im.pel_mask == 15);
    for (int i = 0; i < 64; i++) assert(p[i] == 0x7e && p[size + 64 + i] == 0x7e);
    for (int y = 0; y < h; y++) for (size_t x = (size_t)w; x < stride; x++) assert(im.index[(size_t)y * stride + x] == 0x7e);
    if (cx == 0 && cy == 0 && options->hud == ENH_HUD_NONE) {
        assert(im.index[40 * stride + 320] == 1); /* cel continues past original right edge */
        assert(im.index[40 * stride] == 1);       /* left-clipped cel carry */
        assert(im.index[32 * stride + 336] == 6); /* full-world foreground in margin */
        if (w > 520) assert(im.index[100 * stride + 520] == 0);
    }
    if (options->hud != ENH_HUD_NONE) {
        int ox = options->hud == ENH_HUD_LEFT ? 0 : options->hud == ENH_HUD_RIGHT ? w-320 : (w-320)/2;
        assert(im.index[(size_t)(h-24)*stride + ox + 10] == view.hud[10]);
    }
    free(saved); free(p);
}
int main(void)
{
    level.epoch=1; level.columns=256; level.rows=16; level.policy.end_column=32; level.cels=cels;
    memset(level.map,1,sizeof level.map); memset(level.tiles[1],5,256); memset(level.tiles[2],5,256); memset(level.front[2],6,256);
    level.flags[2]=0x40; level.map[2*256+21]=2;
    for(int y=0;y<256;y++) map.rows[y]=level.map+y*256;
    cels[0].w=16; cels[0].h=8; cels[0].row=2; cels[0].size=16; cels[0].blocks=blocks;
    memset(blocks,255,32); /* coverage then colour channel zero */
    view.valid=1; view.world_valid=1; view.assets=&level; view.map=&map; view.level_epoch=1; view.foreground_enabled=1;
    view.sprite_count=3;
    view.sprites[0].id=1; view.sprites[0].mode=1; view.sprites[0].screen_x=318; view.sprites[0].world_x=318;
    view.sprites[1].id=2; view.sprites[1].mode=1; view.sprites[1].screen_x=-2; view.sprites[1].world_x=-2;
    view.sprites[2].id=3; view.sprites[2].mode=1; view.sprites[2].screen_x=340; view.sprites[2].world_x=340;
    for(int i=0;i<3;i++) {view.sprites[i].screen_y=40; view.sprites[i].world_y=64;}
    for(int i=0;i<320*24;i++) view.hud[i]=(uint8_t)(i%16);
    for(int i=0;i<256;i++) {view.animation[i]=(uint8_t)i;palette[i][0]=(uint8_t)(i%64);}
    const int widths[]={480,640,864}; unsigned cases=0;
    for(unsigned k=0;k<3;k++) for(int b=0;b<3;b++) for(int hud=0;hud<4;hud++) {
        EnhOptions o={(EnhBackdrop)b,(EnhHud)hud,NULL,0};
        run(widths[k],hud ? 288 : 176,&o,0,0); cases++;
        run(widths[k],hud ? 288 : 176,&o,-17.25,31.5); cases++;
        run(widths[k],hud ? 288 : 176,&o,4000.75,-20.5); cases++;
    }
    EnhPosition position={1,400,64}; EnhOptions o={ENH_BACK_MIRROR,ENH_HUD_NONE,&position,1};
    run(864,176,&o,0.5,0.25); cases++;
    view.sprites[0].mode=0; run(864,176,&o,0.5,0.25); cases++;
    view.sprites[0].mode=2; view.sprites[0].mirror=1; run(864,176,&o,0.5,0.25); cases++;
    uint8_t output[864*176]; EnhImage im={0}; im.index=output;im.stride=864;im.capacity=sizeof output;
    view.sprites[0].mode=1; view.sprites[0].mirror=0;
    assert(!enh_render(&view,&level,palette,15,0,0,864,176,&o,&im));
    assert(output[40*864+400]==1 && output[40*864+320]==5);
    view.sprites[0].mode=0;
    assert(!enh_render(&view,&level,palette,15,0,0,864,176,&o,&im)); assert(output[40*864+400]==0);
    view.sprites[0].mode=2;
    assert(!enh_render(&view,&level,palette,15,0,0,864,176,&o,&im)); assert(output[40*864+400]==15);
    view.camera_x=64;view.sprite_count=1;view.sprites[0].world_x=340;view.sprites[0].screen_x=276;view.sprites[0].mode=1;
    view.foreground_count=1;view.foreground[0].cell=2*256+21;view.foreground[0].tile=2;
    assert(!enh_render(&view,&level,palette,15,64,0,864,176,&o,&im));
    assert(output[32*864+272]==5 && output[40*864+336]==1); /* no stale foreground after movement */
    view.camera_x=0;view.foreground_count=0;
    memset(level.map,0,256); level.tile_class[0]=1;
    for(int x=0;x<320;x++) level.backdrop[x]=(uint8_t)(x%16+1);
    o.positions=NULL; o.position_count=0;
    o.backdrop=ENH_BACK_BLACK;
    assert(!enh_render(&view,&level,palette,15,0,0,864,176,&o,&im)); assert(output[0]==0 && output[432]==1);
    o.backdrop=ENH_BACK_MIRROR;
    assert(!enh_render(&view,&level,palette,15,0,0,864,176,&o,&im)); assert(output[0]==16);
    o.backdrop=ENH_BACK_STRETCH;
    assert(!enh_render(&view,&level,palette,15,0,0,864,176,&o,&im)); assert(output[0]==1);
    assert(enh_render(&view,&level,palette,15,NAN,0,864,176,&o,&im)==-1);
    im.capacity--; assert(enh_render(&view,&level,palette,15,0,0,864,176,&o,&im)==-1);
    printf("enh_wide: %u deterministic guarded renders at 480/640/864, all policies, fractional/outside cameras, three sprite modes\n",cases);
    return 0;
}
