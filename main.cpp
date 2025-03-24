#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include <iostream>
#include <filesystem>
#include <string>
#include <sstream>

#include "renderer/SDL3/clay_renderer_SDL3.c"
#include "Simulation.hpp"

static const Uint32 FONT_ID = 0;

static const Clay_Color COLOR_BLACK     = (Clay_Color) {0, 0, 0, 255};
static const Clay_Color COLOR_WHITE     = (Clay_Color) {255, 255, 255, 255};
static const Clay_Color COLOR_GREY      = (Clay_Color) {43, 41, 51, 255};
static const Clay_Color COLOR_BLUE      = (Clay_Color) {111, 173, 162, 255};
static const Clay_Color COLOR_LIGHT     = (Clay_Color) {224, 215, 210, 255};

typedef struct AppState {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Window* debugWindow;
    SDL_Renderer* debugRenderer;
    std::unique_ptr<Simulation> simulation;
} AppState;

static inline Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, uintptr_t userData) {
    TTF_Font *font = gFonts[config->fontId];
    int width, height;

    if(!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return (Clay_Dimensions) {(float) width, (float) height};
}

static void handleAddFood(Clay_ElementId elementID, Clay_PointerData pointerData, int* userData) {
    if(pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {

    }
}

static Clay_RenderCommandArray Clay_CreateLayout(const int windowWidth, const int windowHeight) {
    Clay_BeginLayout();
    CLAY(CLAY_ID("SideBar"),
         CLAY_LAYOUT({
             .sizing = {
                 .width = {200.0f},
                 .height = CLAY_SIZING_GROW()
             },
             .padding = {.left = 4, .right = 4, .top = 8},
             .childGap = 10,
             .childAlignment = {
                     .x = CLAY_ALIGN_X_CENTER,
                     .y = CLAY_ALIGN_Y_TOP
             },
             .layoutDirection = CLAY_TOP_TO_BOTTOM,
         }),
         CLAY_RECTANGLE({
             .color = COLOR_GREY
         })
         ) {
        CLAY(CLAY_ID("Add_Food_Container"),
             CLAY_LAYOUT({
                 .sizing = {
                         .width = {100.0f},
                         .height = {20.0f}
                 },
                 .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                 .childAlignment = {
                         .x = CLAY_ALIGN_X_CENTER,
                         .y = CLAY_ALIGN_Y_CENTER
                 }
             }),
             CLAY_RECTANGLE({
                 .color = COLOR_LIGHT
             })
             ) {
            CLAY(CLAY_ID("Add_Food_Text"),
                 CLAY_TEXT(CLAY_STRING("Add Food"),
                           CLAY_TEXT_CONFIG({
                               .textColor = COLOR_BLACK,
                               .fontId = FONT_ID,
                               .fontSize = 10,
                               }))
                 ) {}
        }
    }
    return Clay_EndLayout();
}

static Clay_RenderCommandArray Clay_Debug_CreateLayout(const int windowWidth, const int windowHeight, const std::string& FPS) {
    Clay_BeginLayout();

    CLAY(CLAY_ID("Debug Container"), CLAY_LAYOUT({
        .sizing = {
        .width = {200.0f},
        .height = CLAY_SIZING_GROW()
        },
        .padding = {.left = 4, .right = 4, .top = 8},
        .childGap = 10,
        .childAlignment = {
            .x = CLAY_ALIGN_X_CENTER,
            .y = CLAY_ALIGN_Y_TOP
        },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
    }),
    CLAY_RECTANGLE({
        .color = COLOR_BLACK
    })) {
        CLAY(CLAY_ID("FPS_Container"),
            CLAY_LAYOUT({
                .sizing = {.width = {100.0f}, .height = {20.0f}},
                .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
            }),
            CLAY_RECTANGLE({
                .color = COLOR_LIGHT
            })
        ) {
            CLAY(CLAY_ID("FPS"),
                CLAY_TEXT(((Clay_String) {.length = static_cast<int32_t>(FPS.length()), .chars = FPS.c_str()}),
                CLAY_TEXT_CONFIG({
                    .textColor = COLOR_BLACK,
                    .fontId = FONT_ID,
                    .fontSize = 10,
                })
            )) {

            }
        }
    }


    return Clay_EndLayout();
}

void HandleClayErrors(Clay_ErrorData errorData) {
    std::cout << errorData.errorText.chars << std::endl;
    switch(errorData.errorType) {
        //...
    }
}

static float getDeltaTime() {
    static uint64_t NOW = SDL_GetPerformanceCounter();
    uint64_t LAST = NOW;

    NOW = SDL_GetPerformanceCounter();
    return static_cast<float>(NOW - LAST) / static_cast<float>(SDL_GetPerformanceFrequency());
}



SDL_AppResult SDL_AppInit(void **appstate, int argc, char* argv[]) {
    //(void) argc;
    //(void) argv;

    if(!TTF_Init()) {
        return SDL_APP_FAILURE;
    }

    auto *state = static_cast<AppState *>(SDL_calloc(1, sizeof(AppState)));
    if(!state) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate state memory: %s", SDL_GetError());

        return SDL_APP_FAILURE;
    }
    //*appstate = state;

    if(!SDL_CreateWindowAndRenderer("Evolution Simulation", 640, 480, 0, &state->window, &state->renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create window and renderer: %s", SDL_GetError());

        SDL_free(state);
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowResizable(state->window, true);
    SDL_SetWindowMinimumSize(state->window, 640, 480);

    #ifndef NDEBUG
        if(!SDL_CreateWindowAndRenderer("Debug Info", 400, 480, 0, &state->debugWindow, &state->debugRenderer)) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create debug window and renderer: %s", SDL_GetError());
        }else {
            SDL_SetWindowResizable(state->debugWindow, false);
            SDL_SetWindowMinimumSize(state->debugWindow, 400, 480);
            SDL_SetWindowPosition(state->debugWindow, 200, 100);
        }
    #endif

    const char *basePath = SDL_GetBasePath();

    std::filesystem::path base(basePath);
    std::filesystem::path fontPath = base / "../resources/arial.ttf";
    std::string fontStr = fontPath.lexically_normal().string();

    TTF_Font *font = TTF_OpenFont(fontStr.c_str(), 24);
    if(!font) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font: %s", SDL_GetError());
        if (state->renderer)
            SDL_DestroyRenderer(state->renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        #ifndef NDEBUG
            SDL_DestroyRenderer(state->debugRenderer);
            SDL_DestroyWindow(state->window);
        #endif

        SDL_free(state);
        return SDL_APP_FAILURE;
    }

    gFonts[FONT_ID] = font;


    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .capacity = totalMemorySize,
        .memory = static_cast<char *>(SDL_malloc(totalMemorySize))
    };

    int width, height;
    SDL_GetWindowSize(state->window, &width, &height);
    Clay_Initialize(
            clayMemory,
            (Clay_Dimensions)
                {(float) width, (float) height},
                (Clay_ErrorHandler) {HandleClayErrors});
    Clay_SetMeasureTextFunction(SDL_MeasureText, 0);

    state->simulation = std::make_unique<Simulation>(SDL_Rect {200, 0, width - 200, height + 0}, 4000, 10);
    *appstate = state;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    SDL_AppResult result = SDL_APP_CONTINUE;

    switch(event->type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            SDL_Event quitEvent;
            quitEvent.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quitEvent);
            break;
        case SDL_EVENT_QUIT:
            result = SDL_APP_SUCCESS;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            Clay_SetLayoutDimensions((Clay_Dimensions) {(float) event->window.data1, (float) event->window.data2});
            break;
        case SDL_EVENT_MOUSE_MOTION:
            Clay_SetPointerState((Clay_Vector2) {event->motion.x, event->motion.y}, event->motion.state & SDL_BUTTON_LEFT);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            Clay_UpdateScrollContainers(true, (Clay_Vector2) {event->motion.xrel, event->motion.yrel}, 0.01f);
            break;
        default:
            break;
    }

    return result;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    float deltaTime = getDeltaTime();
    const auto *state = static_cast<AppState*>(appstate);
    static float timeAccumForFixedUpdate = 0.0f;
    static float timeAccumForFPS = 0.0f;
    std::stringstream stream;
    static std::string FPS = "FPS: 0.00";


    if(timeAccumForFPS >= 1.0f) {
        stream << std::fixed << std::setprecision(2) << (1.0f / deltaTime);
        FPS = "FPS: " + stream.str();
        stream.clear();
        timeAccumForFPS = 0.0f;
    }else timeAccumForFPS += deltaTime;

    int width, height;
    SDL_GetWindowSize(state->window, &width, &height);

    if(timeAccumForFixedUpdate >= 0.016f) {
        state->simulation->fixedUpdate();
        timeAccumForFixedUpdate = 0.0f;
    }else timeAccumForFixedUpdate += deltaTime;

    state->simulation->update(SDL_Rect {200, 0, width - 200, height + 0}, deltaTime);
    Clay_RenderCommandArray renderCommands = Clay_CreateLayout(width, height);

    SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255);
    SDL_RenderClear(state->renderer);

    state->simulation->render(state->renderer);
    SDL_RenderClayCommands(state->renderer, &renderCommands);

    SDL_RenderPresent(state->renderer);

    #ifndef NDEBUG
        int debugWidth, debugHeight;
        SDL_GetWindowSize(state->debugWindow, &debugWidth, &debugHeight);
        Clay_RenderCommandArray debugRenderCommands = Clay_Debug_CreateLayout(debugWidth, debugHeight, FPS);
        SDL_SetRenderDrawColor(state->debugRenderer, 255, 255, 255, 255);
        SDL_RenderClear(state->debugRenderer);
        SDL_RenderClayCommands(state->debugRenderer, &debugRenderCommands);
        SDL_RenderPresent(state->debugRenderer);
    #endif

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    //(void) result

    if(result != SDL_APP_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Application failed to run");
    }

    auto *state = static_cast<AppState*>(appstate);

    if(state) {
        //renderer is already freed?
        if(state->window) {
            SDL_DestroyWindow(state->window);
        }
        if(state->debugWindow) {
            SDL_DestroyWindow(state->debugWindow);
        }
        SDL_free(state);
    }
    TTF_CloseFont(gFonts[FONT_ID]);
    TTF_Quit();
    SDL_Quit();
}

