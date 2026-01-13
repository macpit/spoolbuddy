/**
 * SpoolBuddy LVGL 9.x Simulator with SDL2
 * Display: 800x480 (same as CrowPanel 7.0")
 *
 * This simulator can connect to the real Python backend for testing
 * UI changes without flashing the ESP32 firmware.
 *
 * Usage:
 *   ./simulator                         # Uses default localhost:3000
 *   ./simulator http://192.168.1.10:3000  # Custom backend URL
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "sim_control.h"

#ifdef ENABLE_BACKEND_CLIENT
#include "backend_client.h"
#endif

#define DISP_HOR_RES 800
#define DISP_VER_RES 480

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static uint32_t *fb_pixels;

static lv_display_t *disp;
static lv_indev_t *mouse_indev;

static pthread_mutex_t lvgl_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Display flush callback */
static void sdl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    int32_t x, y;
    uint16_t *src = (uint16_t *)px_map;

    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            uint16_t c = *src++;
            /* Convert RGB565 to ARGB8888 */
            uint8_t r = ((c >> 11) & 0x1F) << 3;
            uint8_t g = ((c >> 5) & 0x3F) << 2;
            uint8_t b = (c & 0x1F) << 3;
            fb_pixels[y * DISP_HOR_RES + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }

    lv_display_flush_ready(display);
}

/* Mouse read callback */
static void sdl_mouse_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    int x, y;
    uint32_t buttons = SDL_GetMouseState(&x, &y);

    data->point.x = x;
    data->point.y = y;
    data->state = (buttons & SDL_BUTTON(1)) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/* Initialize SDL */
static int sdl_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow(
        "SpoolBuddy Simulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DISP_HOR_RES, DISP_VER_RES,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);  /* Use any available renderer */
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return -1;
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        DISP_HOR_RES, DISP_VER_RES
    );
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return -1;
    }

    fb_pixels = malloc(DISP_HOR_RES * DISP_VER_RES * sizeof(uint32_t));
    if (!fb_pixels) {
        fprintf(stderr, "Failed to allocate framebuffer\n");
        return -1;
    }
    memset(fb_pixels, 0, DISP_HOR_RES * DISP_VER_RES * sizeof(uint32_t));

    return 0;
}

/* Cleanup SDL */
static void sdl_deinit(void)
{
    if (fb_pixels) free(fb_pixels);
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

/* LVGL tick thread */
static void *tick_thread(void *arg)
{
    (void)arg;
    while (1) {
        usleep(5000); /* 5ms */
        lv_tick_inc(5);
    }
    return NULL;
}

#ifdef ENABLE_BACKEND_CLIENT
/* Backend polling thread */
static int backend_running = 1;
static pthread_mutex_t backend_mutex = PTHREAD_MUTEX_INITIALIZER;

// External functions for device state (NFC/scale)
extern bool nfc_tag_present(void);
extern uint8_t nfc_get_uid_hex(uint8_t *buf, uint8_t buf_len);
extern float scale_get_weight(void);
extern bool scale_is_stable(void);

static void *backend_thread(void *arg)
{
    (void)arg;
    printf("[backend] Polling thread started\n");

    while (backend_running) {
        pthread_mutex_lock(&backend_mutex);
        int result = backend_poll();

        // Send device state (NFC tag, scale weight) to backend
        float weight = scale_get_weight();
        bool stable = scale_is_stable();
        char tag_id[32] = {0};
        if (nfc_tag_present()) {
            nfc_get_uid_hex((uint8_t*)tag_id, sizeof(tag_id));
        }
        backend_send_device_state(weight, stable, tag_id[0] ? tag_id : NULL);

        pthread_mutex_unlock(&backend_mutex);

        if (result == 0) {
            const BackendState *state = backend_get_state();
            if (state->printer_count > 0) {
                printf("[backend] %d printer(s), first: %s (%s)\n",
                       state->printer_count,
                       state->printers[0].name,
                       state->printers[0].connected ? "connected" : "disconnected");
            }
        }

        // Poll every 2 seconds (like the real firmware)
        usleep(BACKEND_POLL_INTERVAL_MS * 1000);
    }

    printf("[backend] Polling thread stopped\n");
    return NULL;
}
#endif

/* Initialize LVGL display */
static void lvgl_display_init(void)
{
    static uint8_t buf1[DISP_HOR_RES * 100 * 2]; /* 100 lines buffer */

    disp = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    lv_display_set_flush_cb(disp, sdl_flush_cb);
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

/* Initialize LVGL input (mouse) */
static void lvgl_input_init(void)
{
    mouse_indev = lv_indev_create();
    lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(mouse_indev, sdl_mouse_read_cb);
}

/* Render to SDL */
static void sdl_render(void)
{
    SDL_UpdateTexture(texture, NULL, fb_pixels, DISP_HOR_RES * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("===========================================\n");
    printf("  SpoolBuddy LVGL 9 Simulator\n");
    printf("===========================================\n");
    printf("Display: %dx%d\n", DISP_HOR_RES, DISP_VER_RES);

#ifdef ENABLE_BACKEND_CLIENT
    const char *backend_url = BACKEND_DEFAULT_URL;
    if (argc > 1) {
        backend_url = argv[1];
    }
    printf("Backend: %s\n", backend_url);
#else
    printf("Backend: disabled (offline mode)\n");
#endif
    printf("\n");

    /* Initialize SDL */
    if (sdl_init() != 0) {
        return 1;
    }

#ifdef ENABLE_BACKEND_CLIENT
    /* Initialize backend client */
    if (backend_init(backend_url) != 0) {
        fprintf(stderr, "Warning: Backend init failed, running in offline mode\n");
    }
#endif

    /* Initialize LVGL */
    lv_init();
    lvgl_display_init();
    lvgl_input_init();

    /* Start tick thread */
    pthread_t tick_tid;
    pthread_create(&tick_tid, NULL, tick_thread, NULL);

#ifdef ENABLE_BACKEND_CLIENT
    /* Start backend polling thread */
    pthread_t backend_tid;
    pthread_create(&backend_tid, NULL, backend_thread, NULL);
#endif

    /* Initialize UI */
    ui_init();

    printf("UI initialized. Starting main loop...\n");
    sim_print_help();

    /* Main loop */
    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                    case SDLK_n:
                        // Toggle NFC tag present
                        sim_set_nfc_tag_present(!sim_get_nfc_tag_present());
                        break;
                    case SDLK_PLUS:
                    case SDLK_EQUALS:
                    case SDLK_KP_PLUS:
                        // Increase scale weight by 50g
                        sim_set_scale_weight(sim_get_scale_weight() + 50.0f);
                        printf("[sim] Scale weight: %.1fg\n", sim_get_scale_weight());
                        break;
                    case SDLK_MINUS:
                    case SDLK_KP_MINUS:
                        // Decrease scale weight by 50g
                        {
                            float new_weight = sim_get_scale_weight() - 50.0f;
                            if (new_weight < 0) new_weight = 0;
                            sim_set_scale_weight(new_weight);
                            printf("[sim] Scale weight: %.1fg\n", sim_get_scale_weight());
                        }
                        break;
                    case SDLK_s:
                        // Toggle scale initialized
                        {
                            extern bool scale_is_initialized(void);
                            bool current = scale_is_initialized();
                            sim_set_scale_initialized(!current);
                            printf("[sim] Scale %s\n", !current ? "INITIALIZED" : "DISABLED");
                        }
                        break;
                    case SDLK_h:
                        sim_print_help();
                        break;
                }
            }
        }

        pthread_mutex_lock(&lvgl_mutex);
        lv_task_handler();
        ui_tick();  /* Process navigation and screen changes */
        pthread_mutex_unlock(&lvgl_mutex);

        sdl_render();
        usleep(5000); /* ~200 fps max */
    }

    /* Cleanup */
#ifdef ENABLE_BACKEND_CLIENT
    backend_running = 0;
    pthread_join(backend_tid, NULL);
    backend_cleanup();
#endif

    sdl_deinit();
    printf("Simulator exited.\n");

    return 0;
}
