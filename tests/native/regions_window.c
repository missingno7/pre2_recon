#include "native/level_regions.h"
#include "native/presenter.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static FrameGameView view;
static EnhLevel assets;
static void check(int w, int h, double x, double y)
{
    Pre2PresentWindow a=pre2_present_window(&view,x,y,w,h);
    Pre2WorldRect b=a.region.bounds;
    assert(a.visible.x0>=b.x0 && a.visible.x1<=b.x1);
    assert(a.visible.y0>=b.y0 && a.visible.y1<=b.y1);
    if (b.x1-b.x0>=w) { assert(a.x>=b.x0 && a.x+w<=b.x1); assert(a.visible.x1-a.visible.x0==w); }
    else { assert(fabs((a.x+w/2.0)-(b.x0+b.x1)/2.0)<1e-8); assert(a.visible.x0==b.x0 && a.visible.x1==b.x1); }
    if (b.y1-b.y0>=h) { assert(a.y>=b.y0 && a.y+h<=b.y1); assert(a.visible.y1-a.visible.y0==h); }
    else { assert(fabs((a.y+h/2.0)-(b.y0+b.y1)/2.0)<1e-8); assert(a.visible.y0==b.y0 && a.visible.y1==b.y1); }
}
int main(void)
{
    static const unsigned rows[]={49,104,49,45,128,128,128,86,110,12,24,51,51,38,173,84};
    const int widths[]={320,356,468,640}, heights[]={176,240,400};
    view.assets=&assets; view.view_w=320; view.view_h=176; view.camera_limit_column=236;
    unsigned checks=0;
    for (unsigned l=0;l<16;l++) {
        view.level=(uint8_t)l; assets.rows=(uint16_t)rows[l]; view.level_flags=l==5?6:0;
        view.camera_x=view.camera_y=0;
        for (unsigned w=0;w<4;w++) for (unsigned h=0;h<3;h++)
            for (int x=-100;x<4200;x+=31) for (int y=-100;y<(int)rows[l]*16+100;y+=79) {
                check(widths[w],heights[h],x,y); checks++;
            }
    }
    view.level=5; view.level_flags=0; assets.rows=128; view.camera_x=35*16; view.camera_y=114*16;
    Pre2PresentWindow lower=pre2_present_window(&view,35*16-74,114*16,468,176);
    assert(lower.x==35*16 && lower.visible.x1==35*16+468);
    view.camera_x=46*16;
    lower=pre2_present_window(&view,46*16-74,114*16,468,176);
    assert(lower.x==66*16-468); /* original window sits at the right, not centred */
    view.camera_x=34*16;
    lower=pre2_present_window(&view,view.camera_x,view.camera_y,320,176);
    assert(lower.x==34*16); /* original pan into void: preserve the 320 endpoint */
    lower=pre2_present_window(&view,view.camera_x-18,view.camera_y,356,176);
    assert(lower.x==35*16 && lower.region.id==80); /* lower, not the tower */
    unsigned n; const Pre2CaveRegion *c=pre2_region_caves(&n);
    for (unsigned i=0;i<n;i++) {
        view.level=c[i].level; assets.rows=(uint16_t)rows[view.level]; view.room=c[i].gate;
        view.camera_x=(c[i].camera&255)*16; view.camera_y=(c[i].camera>>8)*16;
        Pre2AuthoredRegion r=pre2_region_for_view(&view);
        assert(r.evidence && *r.evidence);
        assert(r.bounds.x0<=view.camera_x && r.bounds.x1>=view.camera_x+320);
        assert(r.bounds.y0<=view.camera_y && r.bounds.y1>=view.camera_y+176);
        for (unsigned w=0;w<4;w++) check(widths[w],176,view.camera_x-(widths[w]-320)/2.0,view.camera_y);
    }
    view.level=0;assets.rows=49;view.room=0;view.level_flags=2;view.camera_x=512;view.camera_y=160;
    Pre2AuthoredRegion horizontal=pre2_region_for_view(&view);
    assert(!horizontal.fixed_screen && horizontal.bounds.x0==512 && horizontal.bounds.x1==832);
    assert(horizontal.bounds.y0==0 && horizontal.bounds.y1==784);
    view.level=13;assets.rows=38;view.level_flags=0;view.camera_x=0;view.camera_y=176;
    Pre2PresentWindow overshoot=pre2_present_window(&view,0,176,320,176);
    assert(overshoot.y==176 && overshoot.visible.y1==352 && !overshoot.region.proven_extent);
    overshoot=pre2_present_window(&view,-74,176,468,176);
    assert(overshoot.y==160 && overshoot.visible.y1==336); /* never widen the staging bank */
    view.level=0;view.camera_x=view.camera_y=0;view.level_epoch=1;assets.rows=49;view.sprite_count=1;
    view.sprites[0]=(FrameSprite){.slot=1,.id=129,.world_x=160,.world_y=88};
    PresenterPose pose={0};pose.view=&view;PresenterCamera camera={0};double px,py;
    presenter_camera(&camera,&pose,0,320,.12,&px,&py);
    view.level=5;assets.rows=128;view.camera_x=600;view.camera_y=100;pose.x=600;pose.y=100;
    view.sprites[0].id=129u^(80u*0x9e3779b9u);view.sprites[0].world_x=view.sprites[0].world_y=1000;
    presenter_camera(&camera,&pose,1000000000,320,.12,&px,&py);
    assert(px==600 && py==100); /* region change must reseed even for colliding identity hashes */
    printf("regions_window: %u edge/size checks; %u authored cave arrivals; lower tower rests at both edges; horizontal lock retains vertical region\n",checks,n);
    return 0;
}
