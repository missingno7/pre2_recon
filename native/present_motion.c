#include "presenter.h"
#include <math.h>
#include <string.h>

static double clamp(double x, double lo, double hi) { return x < lo ? lo : x > hi ? hi : x; }
void presenter_interpolate(const FrameGameView *a, const FrameGameView *b, double t, PresenterPose *o)
{
    memset(o, 0, sizeof *o);
    t = clamp(t, 0, 1); o->alpha = t;
    o->view = t == 0 ? a : b;
    o->x = o->view->camera_x; o->y = o->view->camera_y;
    if (t == 0 || t == 1 || b->camera_cut || a->level_epoch != b->level_epoch || a->assets != b->assets) return;
    o->x = a->camera_x + (b->camera_x - a->camera_x) * t;
    o->y = a->camera_y + (b->camera_y - a->camera_y) * t;
    for (unsigned j = 0; j < b->sprite_count && j < FRAME_MAX_SPRITES; j++) {
        const FrameSprite *s = &b->sprites[j];
        if (s->screen_space) continue;
        for (unsigned i = 0; i < a->sprite_count && i < FRAME_MAX_SPRITES; i++) {
            const FrameSprite *old = &a->sprites[i];
            if (old->id != s->id || old->gen != s->gen || old->screen_space) continue;
            double dx = s->world_x - old->world_x, dy = s->world_y - old->world_y;
            if ((dx || dy) && dx*dx + dy*dy <= PRESENTER_TELEPORT_PX * PRESENTER_TELEPORT_PX) {
                EnhPosition *p = &o->positions[o->count++]; p->id = s->id;
                p->x = old->world_x + dx*t; p->y = old->world_y + dy*t;
            }
            break;
        }
    }
}
int presenter_fixed_aperture(const FrameGameView *v, int width)
{
    EnhLevelPolicy policy = enh_level_policy(v->level);
    /* The audit proves these authored stages. 05h's lower encounter moves its
     * original aperture; its unannotated adjoining bands are not wide scenery. */
    return v->room || (v->level_flags & 2) || policy.fixed_room ||
           v->level == 5 || v->level == 9 || v->level == 14 ||
           (policy.end_column - policy.first_column) * 16 < width ||
           (v->camera_limit_column + 20) * 16 < width;
}
void presenter_camera(PresenterCamera *s, const PresenterPose *p, uint64_t t,
                      int w, double tau, double *x, double *y)
{
    const FrameGameView *v = p->view;
    const FrameSprite *player = NULL;
    for (unsigned i = 0; i < v->sprite_count && i < FRAME_MAX_SPRITES; i++)
        if (v->sprites[i].slot == 1 && !v->sprites[i].screen_space) { player = &v->sprites[i]; break; }
    int fixed = presenter_fixed_aperture(v, w);
    double sw = v->view_w > 320 ? v->view_w : 320;      /* the simulated view's width (wide variant) */
    double tx = p->x - (w - sw) / 2.0, ty = p->y;
    uint32_t id = player ? player->id : 0;
    int seed = !s->seeded || t < s->time || s->epoch != v->level_epoch || s->player_id != id ||
               (v->camera_cut && s->cut_frame != v->sim_frame);
    if (seed) { s->x = tx; s->y = ty; }
    else if (player && !fixed) {
        double px = player->world_x, py = player->world_y;
        for (size_t i = 0; i < p->count; i++) if (p->positions[i].id == player->id) {
            px = p->positions[i].x; py = p->positions[i].y; break;
        }
        /* Anchor centering is a presentation preference, not an inferred body
         * offset. One first-order damp with the actual option on BOTH axes. */
        double k = tau > 0 ? -expm1(-(double)(t - s->time) / 1e9 / tau) : 1;
        s->x += (px - w/2.0 - s->x) * k;
        s->y += (py - 88.0 - s->y) * k;
    } else { s->x = tx; s->y = ty; }
    if (!fixed && v->assets) {
        EnhLevelPolicy policy = enh_level_policy(v->level);
        double lo = policy.first_column * 16.0, end = policy.end_column * 16.0;
        /* CameraLeftLimit is conditional: past its original aperture, retain
         * the original camera's region rather than extending a guessed world. */
        double header_end = (v->camera_limit_column + 20) * 16.0;
        if (v->camera_x + sw <= header_end && end > header_end) end = header_end;
        s->x = clamp(s->x, lo, end - w < lo ? lo : end - w);
        double bottom = v->camera_limit_row * 16.0;
        if (bottom > v->assets->rows * 16.0 - 176) bottom = v->assets->rows * 16.0 - 176;
        s->y = clamp(s->y, 0, bottom < 0 ? 0 : bottom);
    }
    if (fixed) { s->x = tx; s->y = ty; }
    if (v->level_flags & 4) s->y = p->y;
    s->time = t; s->epoch = v->level_epoch; s->player_id = id; s->seeded = 1;
    if (v->camera_cut) s->cut_frame = v->sim_frame;
    *x = s->x; *y = s->y;
    /* Shake is deliberately absent here. enh_render applies captured shake
     * after this camera, once (sprite screen positions already contain it). */
}
