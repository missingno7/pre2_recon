/* Presentation capture (see scene.h). */
#include "pre2.h"
#include "scene.h"
#include <stdarg.h>

void (*pre2_scene_sink)(const SceneEvent *e) = 0;

void scene_emit(uint16_t kind, int n, ...)
{
    if (!pre2_scene_sink) return;
    SceneEvent e;
    e.kind = kind; e.n = (uint16_t)(n > SCENE_MAX_ARGS ? SCENE_MAX_ARGS : n);
    va_list ap; va_start(ap, n);
    for (int i = 0; i < SCENE_MAX_ARGS; i++) e.a[i] = i < e.n ? va_arg(ap, int32_t) : 0;
    va_end(ap);
    pre2_scene_sink(&e);
}
