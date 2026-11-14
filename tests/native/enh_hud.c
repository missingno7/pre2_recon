/* Bounded HUD operations: partial copies, page-local sprite letters and clears.
 * The faithful native strip is an independent reference, poisoned before draw. */
#include "script.h"
#include "native/compositor.h"
#include "native/enh_capture.h"
#include "native/draw_game_assets.h"
#include "c/scene_game.h"
static EnhLevel hud_asset;
static EnhMap hud_map;
static EnhCel hud_cels[1024];
static uint8_t hud_row[256],hud_pixels[64000];
static uint8_t hud_palette[256][3];
static FrameGameView hud_view;
static unsigned checks;
static void check(unsigned page)
{
    hud_view.valid=hud_view.world_valid=1; hud_view.assets=&hud_asset; hud_view.map=&hud_map;
    enh_capture_publish(&hud_view,(uint16_t)page);
    uint8_t expected[320*24]; memcpy(expected,hud_view.hud,sizeof expected);
    memset(hud_view.hud,0xa5,sizeof hud_view.hud);
    for(unsigned mode=ENH_HUD_LEFT;mode<=ENH_HUD_ANCHORED;mode++) {
        EnhOptions options={ENH_BACK_BLACK,(EnhHud)mode,NULL,0};
        EnhImage image={0}; image.index=hud_pixels; image.capacity=sizeof hud_pixels; image.stride=320;
        if(enh_render(&hud_view,&hud_asset,(const uint8_t (*)[3])hud_palette,255,0,0,320,200,&options,&image) ||
                memcmp(hud_pixels+320*176,expected,sizeof expected)) {
            fprintf(stderr,"HUD operation %u page %04X mode %u differs\n",checks,page,mode); exit(1);
        }
        checks++;
    }
}
int main(void)
{
    compositor_init(); hud_asset.rows=16; hud_asset.policy.end_column=256; hud_asset.cels=hud_cels;
    for(unsigned y=0;y<256;y++) hud_map.rows[y]=hud_row;
    for(unsigned i=0;i<65536;i++) game_hud[i]=hud_asset.hud_source[i]=(uint8_t)((i*73u+i/31u)^0x96u);
    uint8_t *cel=malloc(65536); if(!cel) return 1;
    for(unsigned i=0;i<65536;i++) cel[i]=(uint8_t)(i*13u+i/47u);
    game_cels[0xf2].blocks=cel; hud_cels[0xf2].blocks=cel;
    scene_emit(SCENE_GAME_HUD,3,0,0,0); check(0); check(0x2000);
    scene_emit(SCENE_GAME_COPY,6,0x1b80,0x3b80,0x398,1,0,0); check(0x2000);
    scene_emit(SCENE_GAME_HUD,3,2,7,0x1ced); check(0); check(0x2000);
    scene_emit(SCENE_GAME_HUD,3,4,1,0x3cf1); check(0x2000); check(0);
    scene_emit(SCENE_GAME_COPY,6,0x1ce7,0x3ce7,3,8,40,40); check(0x2000);
    scene_emit(SCENE_GAME_CLEAR,4,0x3cec,2,10,40); check(0x2000);
    scene_emit(SCENE_GAME_HUD,3,1,0,0x2000); check(0x2000);
    scene_emit(SCENE_GAME_HUD,3,3,8,0x3cf9); check(0x2000);
    scene_emit(SCENE_GAME_COPY,6,0x2000,0,8000,1,0,0); check(0);
    free(cel); game_cels[0xf2].blocks=NULL;
    printf("enh_hud: %u exact page/placement checks, partial copy/clear, sprite-letter front write, poisoned strip\n",checks);
    return 0;
}
