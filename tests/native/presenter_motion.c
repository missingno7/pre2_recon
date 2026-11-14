#include "native/presenter.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static FrameGameView a,b;
static EnhLevel level;
static void near(double x,double y) { assert(fabs(x-y)<1e-7); }
int main(void)
{
    a.valid=a.world_valid=b.valid=b.world_valid=1;
    a.level_epoch=b.level_epoch=7; a.assets=b.assets=&level;
    a.camera_x=100; b.camera_x=120; a.camera_y=40; b.camera_y=60;
    a.sprite_count=3; b.sprite_count=4;
    a.sprites[0]=(FrameSprite){.slot=1,.id=129,.gen=1,.world_x=140,.world_y=100};
    a.sprites[1]=(FrameSprite){.slot=2,.id=130,.gen=1,.world_x=20,.world_y=20};
    a.sprites[2]=(FrameSprite){.slot=3,.id=131,.gen=1,.world_x=40,.world_y=40};
    b.sprites[0]=a.sprites[0]; b.sprites[0].world_x=240; b.sprites[0].world_y=180;
    b.sprites[1]=a.sprites[1]; b.sprites[1].id=258; b.sprites[1].gen=2; b.sprites[1].world_x=21;
    b.sprites[2]=(FrameSprite){.slot=4,.id=132,.gen=1,.world_x=60};
    b.sprites[3]=(FrameSprite){.slot=5,.id=133,.gen=1,.world_x=60,.screen_space=1};
    PresenterPose p;
    presenter_interpolate(&a,&b,0,&p); assert(p.view==&a && p.count==0); near(p.x,100);
    presenter_interpolate(&a,&b,1,&p); assert(p.view==&b && p.count==0); near(p.x,120);
    presenter_interpolate(&a,&b,.5,&p); assert(p.view==&b && p.count==1);
    assert(p.positions[0].id==129); near(p.positions[0].x,190); near(p.positions[0].y,140); near(p.x,110); near(p.y,50);
    /* >32 and >64 motion is still one identity. The sole large-jump guard is
     * the named 256px Euclidean threshold, never a slot-distance guess. */
    b.sprites[0].world_x=1000;
    presenter_interpolate(&a,&b,.5,&p); assert(!p.count);
    b.sprites[0].world_x=240; b.camera_cut=1;
    presenter_interpolate(&a,&b,.5,&p); assert(!p.count); near(p.x,120);
    b.camera_cut=0; b.level_epoch=8;
    presenter_interpolate(&a,&b,.5,&p); assert(!p.count); near(p.x,120); b.level_epoch=7;
    level.rows=100; a.camera_limit_column=b.camera_limit_column=100; a.camera_limit_row=b.camera_limit_row=89;
    presenter_interpolate(&a,&b,.5,&p);
    PresenterCamera c={0},d={0}; double x,y,xx,yy;
    for (unsigned i=0;i<100;i++) {
        presenter_camera(&c,&p,1000000000ull+i*7000000ull,356,.12,&x,&y);
        presenter_camera(&d,&p,1000000000ull+i*7000000ull,356,.12,&xx,&yy);
        near(x,xx); near(y,yy); assert(x>=0 && x<=1564 && y>=0 && y<=1424);
    }
    PresenterCamera fast={0},slow={0};
    presenter_camera(&fast,&p,0,320,.03,&x,&y); presenter_camera(&slow,&p,0,320,.5,&xx,&yy);
    presenter_camera(&fast,&p,100000000,320,.03,&x,&y); presenter_camera(&slow,&p,100000000,320,.5,&xx,&yy);
    assert(fabs(x-xx)>1 && fabs(y-yy)>1);
    b.camera_cut=1; b.sim_frame=99; b.camera_x=500; b.camera_y=600;
    presenter_interpolate(&a,&b,.5,&p); presenter_camera(&c,&p,2000000000,320,.12,&x,&y); near(x,500); near(y,600);
    b.level_flags=4; b.camera_cut=0; b.camera_y=620;
    presenter_interpolate(&b,&b,1,&p); presenter_camera(&c,&p,2100000000,320,.12,&x,&y); near(y,620);
    b.room=1; b.camera_x=200;
    presenter_interpolate(&b,&b,1,&p); presenter_camera(&c,&p,2200000000,320,.12,&x,&y); near(x,200); near(y,620);
    assert(presenter_fixed_aperture(&b,356)); b.room=0; b.level=14; assert(presenter_fixed_aperture(&b,356));
    puts("presenter_motion: IDs, generations, spawn/despawn, endpoints, cuts, screen-space, teleport, deterministic camera, strength and region clamps pass");
    return 0;
}
