/* Native SDL3 host for the translated game (skeleton: window, presentation, event pump).
 *
 * The original address space is initialised from the program image of our own ASM
 * build (embedded by tools/hostbuild.py as pre2_image[], loaded at LOAD_SEG with the
 * relocations applied), never from PRE2.EXE. The game entry is wired in as the C
 * translation of ProgramEntry becomes available; until then the host presents the
 * modelled VGA state.
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string.h>
#include "../c/pre2.h"
#include "../c/vga.h"
#include "present.h"

extern const uint8_t pre2_image[];
extern const uint32_t pre2_image_size;
extern const uint32_t pre2_relocations[];
extern const uint32_t pre2_relocation_count;

static void load_image(void)
{
    memset(pre2_mem, 0, MEM_SIZE);
    memcpy(pre2_mem + lin(LOAD_SEG, 0), pre2_image, pre2_image_size);
    for (uint32_t i = 0; i < pre2_relocation_count; i++) {
        uint32_t p = lin(LOAD_SEG, 0) + pre2_relocations[i];
        uint16_t w = (uint16_t)(pre2_mem[p] | pre2_mem[p + 1] << 8);
        w = (uint16_t)(w + LOAD_SEG);
        pre2_mem[p] = (uint8_t)w; pre2_mem[p + 1] = (uint8_t)(w >> 8);
    }
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    load_image();
    vga_reset();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    SDL_Window *win = SDL_CreateWindow("Prehistorik 2", 960, 600, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = win ? SDL_CreateRenderer(win, NULL) : NULL;
    SDL_Texture *tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, 320, 200) : NULL;
    if (!tex) { SDL_Log("SDL setup: %s", SDL_GetError()); return 1; }
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderLogicalPresentation(ren, 320, 200, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    static uint32_t pixels[320 * 200];
    for (int running = 1; running;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = 0;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = 0;
        }
        present_vga(pixels, 320, 200);
        SDL_UpdateTexture(tex, NULL, pixels, 320 * 4);
        SDL_RenderClear(ren);
        SDL_RenderTexture(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
        SDL_Delay(10);
    }
    SDL_Quit();
    return 0;
}
