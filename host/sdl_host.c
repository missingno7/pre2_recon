/* SDL3 window, audio device and keyboard for the native host (see sdl_host.h). */
#include <SDL3/SDL.h>
#include <stdio.h>
#include "sdl_host.h"

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
static SDL_AudioStream *audio;
static int tex_w, tex_h;

/* ---- keyboard: SDL scancode -> PC scan code set 1 ----------------------------------------- */
uint8_t sdl_pc_scancode(int sdl_scancode, int *extended)
{
    *extended = 0;
    SDL_Scancode s = (SDL_Scancode)sdl_scancode;
    switch (s) {
    case SDL_SCANCODE_ESCAPE: return 0x01;
    case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03; case SDL_SCANCODE_3: return 0x04;
    case SDL_SCANCODE_4: return 0x05; case SDL_SCANCODE_5: return 0x06; case SDL_SCANCODE_6: return 0x07;
    case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09; case SDL_SCANCODE_9: return 0x0A;
    case SDL_SCANCODE_0: return 0x0B; case SDL_SCANCODE_MINUS: return 0x0C; case SDL_SCANCODE_EQUALS: return 0x0D;
    case SDL_SCANCODE_BACKSPACE: return 0x0E; case SDL_SCANCODE_TAB: return 0x0F;
    case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11; case SDL_SCANCODE_E: return 0x12;
    case SDL_SCANCODE_R: return 0x13; case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
    case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17; case SDL_SCANCODE_O: return 0x18;
    case SDL_SCANCODE_P: return 0x19; case SDL_SCANCODE_RETURN: return 0x1C; case SDL_SCANCODE_LCTRL: return 0x1D;
    case SDL_SCANCODE_A: return 0x1E; case SDL_SCANCODE_S: return 0x1F; case SDL_SCANCODE_D: return 0x20;
    case SDL_SCANCODE_F: return 0x21; case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
    case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25; case SDL_SCANCODE_L: return 0x26;
    case SDL_SCANCODE_LSHIFT: return 0x2A; case SDL_SCANCODE_Z: return 0x2C; case SDL_SCANCODE_X: return 0x2D;
    case SDL_SCANCODE_C: return 0x2E; case SDL_SCANCODE_V: return 0x2F; case SDL_SCANCODE_B: return 0x30;
    case SDL_SCANCODE_N: return 0x31; case SDL_SCANCODE_M: return 0x32; case SDL_SCANCODE_RSHIFT: return 0x36;
    case SDL_SCANCODE_LALT: return 0x38; case SDL_SCANCODE_SPACE: return 0x39;
    case SDL_SCANCODE_F1: return 0x3B; case SDL_SCANCODE_F2: return 0x3C; case SDL_SCANCODE_F3: return 0x3D;
    case SDL_SCANCODE_F4: return 0x3E; case SDL_SCANCODE_F5: return 0x3F; case SDL_SCANCODE_F6: return 0x40;
    case SDL_SCANCODE_F7: return 0x41; case SDL_SCANCODE_F8: return 0x42; case SDL_SCANCODE_F9: return 0x43;
    case SDL_SCANCODE_F10: return 0x44;
    case SDL_SCANCODE_LEFTBRACKET: return 0x1A; case SDL_SCANCODE_RIGHTBRACKET: return 0x1B;
    case SDL_SCANCODE_SEMICOLON: return 0x27; case SDL_SCANCODE_APOSTROPHE: return 0x28;
    case SDL_SCANCODE_GRAVE: return 0x29; case SDL_SCANCODE_BACKSLASH: return 0x2B;
    case SDL_SCANCODE_COMMA: return 0x33; case SDL_SCANCODE_PERIOD: return 0x34;
    case SDL_SCANCODE_SLASH: return 0x35; case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
    case SDL_SCANCODE_CAPSLOCK: return 0x3A; case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45;
    case SDL_SCANCODE_SCROLLLOCK: return 0x46; case SDL_SCANCODE_F11: return 0x57;
    case SDL_SCANCODE_F12: return 0x58;
    case SDL_SCANCODE_KP_7: return 0x47; case SDL_SCANCODE_KP_8: return 0x48;
    case SDL_SCANCODE_KP_9: return 0x49; case SDL_SCANCODE_KP_MINUS: return 0x4A;
    case SDL_SCANCODE_KP_4: return 0x4B; case SDL_SCANCODE_KP_5: return 0x4C;
    case SDL_SCANCODE_KP_6: return 0x4D; case SDL_SCANCODE_KP_PLUS: return 0x4E;
    case SDL_SCANCODE_KP_1: return 0x4F; case SDL_SCANCODE_KP_2: return 0x50;
    case SDL_SCANCODE_KP_3: return 0x51; case SDL_SCANCODE_KP_0: return 0x52;
    case SDL_SCANCODE_KP_PERIOD: return 0x53;
    /* the extended group: E0h, then the code of the numeric-pad / left-hand key it doubles */
    case SDL_SCANCODE_UP: *extended = 1; return 0x48; case SDL_SCANCODE_LEFT: *extended = 1; return 0x4B;
    case SDL_SCANCODE_RIGHT: *extended = 1; return 0x4D; case SDL_SCANCODE_DOWN: *extended = 1; return 0x50;
    case SDL_SCANCODE_HOME: *extended = 1; return 0x47; case SDL_SCANCODE_PAGEUP: *extended = 1; return 0x49;
    case SDL_SCANCODE_END: *extended = 1; return 0x4F; case SDL_SCANCODE_PAGEDOWN: *extended = 1; return 0x51;
    case SDL_SCANCODE_INSERT: *extended = 1; return 0x52; case SDL_SCANCODE_DELETE: *extended = 1; return 0x53;
    case SDL_SCANCODE_RCTRL: *extended = 1; return 0x1D; case SDL_SCANCODE_RALT: *extended = 1; return 0x38;
    case SDL_SCANCODE_KP_ENTER: *extended = 1; return 0x1C; case SDL_SCANCODE_KP_DIVIDE: *extended = 1; return 0x35;
    default: return 0;
    }
}

int sdl_host_init(int video, int sound)
{
    SDL_InitFlags f = (video ? SDL_INIT_VIDEO : 0) | (sound ? SDL_INIT_AUDIO : 0);
    if (!SDL_Init(f)) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }
    if (video) {
        win = SDL_CreateWindow("Prehistorik 2", 960, 600, SDL_WINDOW_RESIZABLE);
        ren = win ? SDL_CreateRenderer(win, NULL) : NULL;
        if (!ren) { fprintf(stderr, "SDL video: %s\n", SDL_GetError()); return 1; }
        SDL_SetRenderVSync(ren, 1);
    }
    if (sound) {
        SDL_AudioSpec src = { SDL_AUDIO_U8, 1, 8403 };
        audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &src, NULL, NULL);
        if (audio) SDL_ResumeAudioStreamDevice(audio);
    }
    return 0;
}

void sdl_host_shutdown(void)
{
    if (audio) SDL_DestroyAudioStream(audio);
    SDL_Quit();
}

int sdl_host_pump(void (*key)(uint8_t code))
{
    int quit = 0;
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) quit = 1;
        /* key down (also SDL's repeats: the PC keyboard's typematic repeat sends the make
         * code again, and the game never changes the typematic rate) and key up; keys of the
         * 101-key keyboard's extended group are preceded by E0h, as the keyboard sends them */
        if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
            int ext; uint8_t code = sdl_pc_scancode(e.key.scancode, &ext);
            if (code) {
                if (ext) key(0xE0);
                key((uint8_t)(code | (e.type == SDL_EVENT_KEY_UP ? 0x80 : 0)));
            }
        }
    }
    return quit;
}

void sdl_host_show(const uint32_t *xrgb, int w, int h, int aspect_w, int aspect_h)
{
    if (!ren) return;
    if (!tex || w != tex_w || h != tex_h) {
        if (tex) SDL_DestroyTexture(tex);
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
        tex_w = w; tex_h = h;
        SDL_SetRenderLogicalPresentation(ren, aspect_w, aspect_h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    SDL_UpdateTexture(tex, NULL, xrgb, w * 4);
    SDL_RenderClear(ren);
    SDL_RenderTexture(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

void sdl_host_audio(const uint8_t *pcm, uint32_t n, uint32_t rate)
{
    static uint32_t cur = 8403;
    if (!audio) return;
    if (rate != cur) {
        SDL_AudioSpec src = { SDL_AUDIO_U8, 1, (int)rate };
        SDL_SetAudioStreamFormat(audio, &src, NULL);
        cur = rate;
    }
    SDL_PutAudioStreamData(audio, pcm, (int)n);
}

uint32_t sdl_host_audio_queued(void) { return audio ? (uint32_t)SDL_GetAudioStreamQueued(audio) : 0; }
uint64_t sdl_host_ns(void) { return SDL_GetTicksNS(); }
void sdl_host_sleep_ns(uint64_t ns) { SDL_DelayNS(ns); }
