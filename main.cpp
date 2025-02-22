#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include <iostream>
#include <__filesystem/operations.h>

#include "renderer/SDL3/clay_renderer_SDL3.c"
#include "Genome.hpp"
#include "NeuralNet.hpp"

static const Uint32 FONT_ID = 0;

static const Clay_Color COLOR_BLACK     = (Clay_Color) {0, 0, 0, 255};
static const Clay_Color COLOR_WHITE     = (Clay_Color) {255, 255, 255, 255};
static const Clay_Color COLOR_GREY      = (Clay_Color) {43, 41, 51, 255};
static const Clay_Color COLOR_BLUE      = (Clay_Color) {111, 173, 162, 255};
static const Clay_Color COLOR_LIGHT     = (Clay_Color) {224, 215, 210, 255};

typedef struct AppState {
    SDL_Window *window;
    SDL_Renderer *renderer;
} AppState;

static inline Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, uintptr_t userData) {
    TTF_Font *font = gFonts[config->fontId];
    int width, height;

    if(!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return (Clay_Dimensions) {(float) width, (float) height};
}

static Clay_RenderCommandArray Clay_CreateLayout(int windowWidth, int windowHeight) {
    Clay_BeginLayout();
    CLAY(CLAY_ID("SideBar"),
         CLAY_LAYOUT({
             .sizing = {
                 .width = (float)(windowWidth / 5),
                 .height = CLAY_SIZING_GROW()
             },
             .childGap = 10,
             .layoutDirection = CLAY_TOP_TO_BOTTOM,
         }),
         CLAY_RECTANGLE({
             .color = COLOR_GREY
         })
         ) {}
    return Clay_EndLayout();
}

void HandleClayErrors(Clay_ErrorData errorData) {
    std::cout << errorData.errorText.chars << std::endl;
    switch(errorData.errorType) {
        //...
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char* argv[]) {
    //(void) argc;
    //(void) argv;

    Genome::Genome genome1 = Genome::createRandomGenome(1000);
    NeuralNet net(genome1);


    //for(uint32_t connection : genome1.connections) {
    //    std::bitset<32> bits(connection);
    //    std::cout << bits << std::endl;
    //}
    //for(const auto& pair : genome1.biases) {
    //    std::bitset<8> neuronID(pair.first);
    //    std::bitset<16> bias(pair.second);
    //    std::cout << neuronID << " : " << bias << std::endl;
    //}

    Genome::Genome genome2 = Genome::createRandomGenome(10);
    Genome::Genome genome3 = Genome::createGenomeFromParents(genome1, genome2);
    Genome::mutateGenome(&genome1);
    Genome::mutateGenome(&genome2);
    Genome::mutateGenome(&genome3);

    if(!TTF_Init()) {
        return SDL_APP_FAILURE;
    }

    auto *state = static_cast<AppState *>(SDL_calloc(1, sizeof(AppState)));
    if(!state) {
        return SDL_APP_FAILURE;
    }
    //*appstate = state;

    if(!SDL_CreateWindowAndRenderer("Evolution Simulation", 640, 480, 0, &state->window, &state->renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create window and renderer: %s", SDL_GetError());

        SDL_free(state);
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowResizable(state->window, true);

    const char *basePath = SDL_GetBasePath();

    std::filesystem::path base(basePath);
    std::filesystem::path fontPath = base / "../../../../resources/arial.ttf";
    std::string fontStr = fontPath.lexically_normal().string();

    TTF_Font *font = TTF_OpenFont(fontStr.c_str(), 24);
    if(!font) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font: %s", SDL_GetError());
        if (state->renderer)
            SDL_DestroyRenderer(state->renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

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
    Clay_Initialize(clayMemory, (Clay_Dimensions) {(float) width, (float) height}, (Clay_ErrorHandler) {HandleClayErrors});
    Clay_SetMeasureTextFunction(SDL_MeasureText, 0);

    *appstate = state;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    SDL_AppResult result = SDL_APP_CONTINUE;

    switch(event->type) {
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
    const auto *state = static_cast<AppState*>(appstate);

    int width, height;
    SDL_GetWindowSize(state->window, &width, &height);
    Clay_RenderCommandArray renderCommands = Clay_CreateLayout(width, height);

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);
    SDL_RenderClayCommands(state->renderer, &renderCommands);

    SDL_RenderPresent(state->renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    //(void) result

    if(result != SDL_APP_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Application failed to run");
    }

    auto *state = static_cast<AppState*>(appstate);

    if(state) {
        if(state->window) {
            SDL_DestroyWindow(state->window);
        }
        SDL_free(state);
    }
    TTF_CloseFont(gFonts[FONT_ID]);
    TTF_Quit();
}

