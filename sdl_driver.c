#include "config.h"

#include <SDL.h>
#include <stdbool.h>
#include <string.h>
#include <sys/errno.h>

#include "vg8m.h"
#include "video.h"

static const int MS_FRAME = 1000/30;

static VG8M *emu = NULL;

static SDL_Window *window = NULL;
static SDL_Rect window_rect;
static SDL_Palette *hwpalette = NULL;
static SDL_Color hwcolors[1 << 8];

#ifdef USE_TEXTURE
static SDL_Renderer *renderer;
static SDL_Texture *screen;
#else
static SDL_Surface *screen = NULL, *window_surface = NULL;
#endif // USE_TEXTURE

static bool running;

void _input(VG8M *emu, void *_) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        }
        else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            if (!event.key.keysym.mod) {
                VG8MButtonMask button;
                if      (event.key.keysym.sym == SDLK_q)
                    button = VG8M_BUTTON_LT;
                else if (event.key.keysym.sym == SDLK_w)
                    button = VG8M_BUTTON_GAMMA;
                else if (event.key.keysym.sym == SDLK_e)
                    button = VG8M_BUTTON_RT;
                else if (event.key.keysym.sym == SDLK_a)
                    button = VG8M_BUTTON_DELTA;
                else if (event.key.keysym.sym == SDLK_s)
                    button = VG8M_BUTTON_BETA;
                else if (event.key.keysym.sym == SDLK_d)
                    button = VG8M_BUTTON_ALPHA;
                else if (event.key.keysym.sym == SDLK_UP)
                    button = VG8M_BUTTON_UP;
                else if (event.key.keysym.sym == SDLK_DOWN)
                    button = VG8M_BUTTON_DOWN;
                else if (event.key.keysym.sym == SDLK_LEFT)
                    button = VG8M_BUTTON_LEFT;
                else if (event.key.keysym.sym == SDLK_RIGHT)
                    button = VG8M_BUTTON_RIGHT;
                vg8m_set_buttons(emu, button, event.key.state);
            }
        }
    }
}

void _scanline(VG8M *emu, void *_) {
#ifdef USE_TEXTURE
    SDL_Rect scanline_rect;
    scanline_rect.x = 0;
    scanline_rect.y = emu->line;
    scanline_rect.w = VG8M_DISP_WIDTH;
    scanline_rect.h = VG8M_DISP_HEIGHT;

    void *pixels;
    int pitch;

    if (SDL_LockTexture(screen, &scanline_rect, &pixels, &pitch) != 0)
        fputs(SDL_GetError(), stderr);
    vg8m_scanline(emu, pixels);
    SDL_UnlockTexture(screen);
#else
    SDL_LockSurface(screen);
    vg8m_scanline(emu, (uint32_t*)(screen->pixels) + emu->line * VG8M_DISP_WIDTH);
    SDL_UnlockSurface(screen);
#endif // USE_TEXTURE

    // XXX ABSOLUTELY NOT LMAO
    // SDL_BlitSurface(screen, NULL, window_surface, &window_rect);
    // SDL_UpdateWindowSurface(window);
}

long t = 0;
void _display(VG8M *emu, void *_) {
#ifdef USE_TEXTURE
    SDL_RenderCopy(renderer, screen, NULL, NULL);
    SDL_RenderPresent(renderer);
#else
    SDL_BlitScaled(screen, NULL, window_surface, &window_rect);
    SDL_UpdateWindowSurface(window);
#endif // USE_TEXTURE

    long t_last = t;
    t = SDL_GetTicks();
    long dt = t - t_last;
    if (dt < MS_FRAME)
        SDL_Delay(MS_FRAME - dt);
}

int main(int argc, char **argv) {
    int status = 0;

    emu = calloc(1, sizeof(VG8M));
    vg8m_init(emu);

    if (!vg8m_load_system(emu, "bios/system.bin", "bios/charset.1bpp"))
        goto other_error;

    if (argc > 1 && !vg8m_load_cart(emu, argv[1]))
        goto other_error;

    emu->display_callback  = _display;
    emu->scanline_callback = _scanline;
    emu->input_callback    = _input;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        goto sdl_error;

    window = SDL_CreateWindow("VEC-VG8M",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        2 * VG8M_DISP_WIDTH, 2 * VG8M_DISP_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
        goto sdl_error;

#ifdef USE_TEXTURE
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        goto sdl_error;

    screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB32,
        SDL_TEXTUREACCESS_STREAMING, VG8M_DISP_WIDTH, VG8M_DISP_HEIGHT);
    if (!screen)
        goto sdl_error;
#else
    window_surface = SDL_GetWindowSurface(window);
    if (!window_surface)
        goto sdl_error;

    SDL_SetSurfaceBlendMode(window_surface, SDL_BLENDMODE_NONE);
    SDL_SetColorKey(window_surface, 0, 0);
    SDL_SetColorKey(screen, 0, 0);

    window_rect.x = 0;
    window_rect.y = 0;
    window_rect.w = window_surface->w;
    window_rect.h = window_surface->h;

    screen = SDL_CreateRGBSurfaceWithFormat(0,
        VG8M_DISP_WIDTH, VG8M_DISP_HEIGHT, 8, SDL_PIXELFORMAT_ARGB8888);
    if (!screen)
        goto sdl_error;
#endif // USE_TEXTURE

    running = true;
    while (running) {
        vg8m_step_instruction(emu);
    }

cleanup:
    if (emu) {
        vg8m_fin(emu);
        free(emu);
    }

#ifdef USE_TEXTURE
    if (screen)         SDL_DestroyTexture(screen);
    if (renderer)       SDL_DestroyRenderer(renderer);
#else
    if (screen)         SDL_FreeSurface(screen);
#endif

    if (hwpalette)      SDL_FreePalette(hwpalette);
    if (window)         SDL_DestroyWindow(window);

    SDL_Quit();

    return status;

sdl_error:
    fprintf(stderr, "%s\n", SDL_GetError());

    status = 1;
    goto cleanup;

other_error:
    fprintf(stderr, "%s\n", strerror(errno));

    status = 1;
    goto cleanup;
}
