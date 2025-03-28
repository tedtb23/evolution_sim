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
#include <cstring>
#include "renderer/SDL3/clay_renderer_SDL3.c"
#include "Simulation.hpp"
#include "UIStructs.hpp"

static const Uint32 FONT_SMALL = 0;
static const Uint32 FONT_MEDIUM = 1;
static const Uint32 FONT_LARGE= 2;


static const Clay_Color COLOR_BLACK     = (Clay_Color) {0, 0, 0, 255};
static const Clay_Color COLOR_WHITE     = (Clay_Color) {255, 255, 255, 255};
static const Clay_Color COLOR_GREY      = (Clay_Color) {43, 41, 51, 255};
static const Clay_Color COLOR_BLUE      = (Clay_Color) {50, 90, 162, 255};
static const Clay_Color COLOR_LIGHT     = (Clay_Color) {224, 215, 210, 255};

typedef struct AppState {
    SDL_Window* window;
    SDL_Window* debugWindow;
    SDL_Renderer* renderer;
    SDL_Renderer* debugRenderer;
    Clay_SDL3RendererData rendererData;
    Clay_SDL3RendererData debugRendererData;
    std::shared_ptr<Simulation> simulation;
} AppState;

static inline Clay_Dimensions SDL_MeasureText(Clay_StringSlice text, Clay_TextElementConfig *config, void* userData) {
    auto** fonts = static_cast<TTF_Font**>(userData);
    TTF_Font* font = fonts[config->fontId];
    int width, height;
    if(config->fontSize > 0) TTF_SetFontSize(font, config->fontSize);

    if(!TTF_GetStringSize(font, text.chars, text.length, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to measure text: %s", SDL_GetError());
    }

    return (Clay_Dimensions) {(float) width, (float) height};
}

static void handleButtonPress(Clay_ElementId elementID, Clay_PointerData pointerData, intptr_t userData) {
    if(pointerData.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME) {
        const auto simPtr = reinterpret_cast<Simulation*>(userData);
        if(strcmp(elementID.stringId.chars, "Add_Food_Button") == 0) {
            simPtr->setUserAction(UserActionType::ADD_FOOD, UIData{});
        }else if(strcmp(elementID.stringId.chars, "Show_QuadTree_Button") == 0) {
            static bool quadIsShown = false;
            quadIsShown = !quadIsShown;
            simPtr->showQuadTree(quadIsShown);
        }
    }
}

static Clay_RenderCommandArray Clay_CreateLayout(const int windowWidth, const int windowHeight, const std::shared_ptr<Simulation>& simPtr, const std::string& FPS, const std::string& s) {
    Clay_BeginLayout();
    CLAY({
        .id = CLAY_ID("Background"),
        .layout = {
            .sizing = {
                .width = {CLAY_SIZING_GROW()},
                .height = {CLAY_SIZING_GROW()}
            },
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
    }) {
        CLAY({
        .id = CLAY_ID("SideBar"),
        .layout = {
            .sizing = {
                .width = CLAY_SIZING_FIXED(200.0f),
                .height = CLAY_SIZING_GROW(0)
             },
             .padding = {.left = 4, .right = 4, .top = 8},
             .childGap = 10,
             .childAlignment = {
                .x = CLAY_ALIGN_X_CENTER,
                .y = CLAY_ALIGN_Y_TOP
             },
             .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
        .scroll = {.vertical = true},
        .backgroundColor = COLOR_GREY
    }) {
            CLAY({
            .id = CLAY_ID("FPS_Container"),
            .layout = {
                .sizing = {.width = {20.0f}, .height = {20.0f}},
                .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
            },
            .backgroundColor = COLOR_LIGHT
            }) {
                CLAY_TEXT(((Clay_String) {.length = static_cast<int32_t>(FPS.length()), .chars = FPS.c_str()}),
                    CLAY_TEXT_CONFIG({
                        .textColor = COLOR_BLACK,
                        .fontId = FONT_SMALL,
                        .fontSize = 0,
                    })
                    );
            }
            CLAY({
                .id = CLAY_ID("Add_Food_Button"),
                .layout = {
                    .sizing = {
                        .width = {100.0f},
                        .height = {20.0f}
                    },
                    .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                    .childAlignment = {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                    }
                },
                .backgroundColor = Clay_Hovered() ?  COLOR_BLUE : COLOR_LIGHT,
            }) {
                Clay_OnHover(handleButtonPress, reinterpret_cast<intptr_t>(simPtr.get()));
                CLAY_TEXT(CLAY_STRING("Add Food"),
                          CLAY_TEXT_CONFIG({
                              .textColor = COLOR_BLACK,
                              .fontId = FONT_MEDIUM,
                              .fontSize = 0,
                              }));
            }
            CLAY({
                .id = CLAY_ID("Show_QuadTree_Button"),
                .layout = {
                .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                .childAlignment = {
                    .x = CLAY_ALIGN_X_CENTER,
                    .y = CLAY_ALIGN_Y_CENTER,
                }
                },
                .backgroundColor = Clay_Hovered() ?  COLOR_BLUE : COLOR_LIGHT,
            }) {
                Clay_OnHover(handleButtonPress, reinterpret_cast<intptr_t>(simPtr.get()));
                CLAY_TEXT(CLAY_STRING("Show QuadTree"),
                          CLAY_TEXT_CONFIG({
                              .wrapMode = CLAY_TEXT_WRAP_NONE,
                              .textColor = COLOR_BLACK,
                              .fontId = FONT_MEDIUM,
                              .fontSize = 0,
                              }));
            }
            CLAY({
                .layout = {
                    .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                    .childAlignment = {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                    }
                },
                .backgroundColor = COLOR_LIGHT
            }) {
                CLAY_TEXT(((Clay_String){.length = static_cast<int32_t>(s.length()), .chars = s.c_str()}),
                CLAY_TEXT_CONFIG({
                    .textColor = COLOR_BLACK,
                    .fontId = FONT_SMALL,
                    .fontSize = 0,
                })
                );
            }
        }
    }

    return Clay_EndLayout();
}

static Clay_RenderCommandArray Clay_Debug_CreateLayout(const int windowWidth, const int windowHeight, const std::string& FPS) {
    Clay_BeginLayout();

    CLAY({
        .id = CLAY_ID("Debug Container"),
        .layout = {
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
        },
        .backgroundColor = COLOR_BLACK,
    }) {
        CLAY({
            .id = CLAY_ID("FPS_Container"),
            .layout = {
                .sizing = {.width = {100.0f}, .height = {20.0f}},
                .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}
            },
            .backgroundColor = COLOR_LIGHT
        }) {
            CLAY_TEXT(((Clay_String) {.length = static_cast<int32_t>(FPS.length()), .chars = FPS.c_str()}),
                CLAY_TEXT_CONFIG({
                    .textColor = COLOR_BLACK,
                    //.fontId = FONT_ID,
                    .fontSize = 10,
                })
                );
        }
    }

    return Clay_EndLayout();
}

void HandleClayErrors(Clay_ErrorData errorData) {
    std::cout << errorData.errorText.chars << std::endl;
    switch(errorData.errorType) {
        default:
            throw std::runtime_error("Clay Error");
    }
}

static float getDeltaTime() {
    static uint64_t NOW = SDL_GetPerformanceCounter();
    uint64_t LAST = NOW;

    NOW = SDL_GetPerformanceCounter();
    return static_cast<float>(NOW - LAST) / static_cast<float>(SDL_GetPerformanceFrequency());
}



SDL_AppResult SDL_AppInit(void **appstate, int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    if(!TTF_Init()) {
        return SDL_APP_FAILURE;
    }

    auto *statePtr = static_cast<AppState *>(SDL_calloc(1, sizeof(AppState)));
    if(!statePtr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate state memory: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if(!SDL_CreateWindowAndRenderer("Evolution Simulation", 640, 480, 0, &statePtr->window, &statePtr->renderer)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    statePtr->rendererData.renderer = statePtr->renderer;
    SDL_SetWindowResizable(statePtr->window, true);
    SDL_SetWindowMinimumSize(statePtr->window, 640, 480);

    //#ifndef NDEBUG
    //    if(!SDL_CreateWindowAndRenderer("Debug Info", 400, 480, 0, &statePtr->debugWindow, &statePtr->debugRenderer)) {
    //        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create debug window and renderer: %s", SDL_GetError());
    //    }else {
    //        SDL_SetWindowResizable(statePtr->debugWindow, false);
    //        SDL_SetWindowMinimumSize(statePtr->debugWindow, 400, 480);
    //        SDL_SetWindowPosition(statePtr->debugWindow, 200, 100);
    //    }
    //    statePtr->debugRendererData.renderer = statePtr->debugRenderer;
//
    //    statePtr->debugRendererData.textEngine = TTF_CreateRendererTextEngine(statePtr->debugRenderer);
    //    if (!statePtr->debugRendererData.textEngine) {
    //        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create text engine from renderer: %s", SDL_GetError());
    //    }
//
    //    statePtr->debugRendererData.fonts = static_cast<TTF_Font **>(SDL_calloc(1, sizeof(TTF_Font *)));
    //    if (!statePtr->debugRendererData.fonts) {
    //        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate memory for the font array: %s", SDL_GetError());
    //    }
    //#endif

    statePtr->rendererData.textEngine = TTF_CreateRendererTextEngine(statePtr->renderer);
    if (!statePtr->rendererData.textEngine) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create text engine from renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    statePtr->rendererData.fonts = static_cast<TTF_Font **>(SDL_calloc(3, sizeof(TTF_Font *)));
    if (!statePtr->rendererData.fonts) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate memory for the font array: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const char *basePath = SDL_GetBasePath();

    std::filesystem::path base(basePath);
    std::filesystem::path fontPath = base / "../resources/arial.ttf";
    std::string fontStr = fontPath.lexically_normal().string();

    TTF_Font* fontSmall = TTF_OpenFont(fontStr.c_str(), 16);
    TTF_Font* fontMedium = TTF_OpenFont(fontStr.c_str(), 24);
    TTF_Font* fontLarge = TTF_OpenFont(fontStr.c_str(), 30);


    if(!fontSmall || !fontMedium || !fontLarge) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    statePtr->rendererData.fonts[FONT_SMALL] = fontSmall;
    statePtr->rendererData.fonts[FONT_MEDIUM] = fontMedium;
    statePtr->rendererData.fonts[FONT_LARGE] = fontLarge;

    //#ifndef NDEBUG
    //statePtr->debugRendererData.fonts[FONT_ID] = font;
    //#endif

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .capacity = totalMemorySize,
        .memory = static_cast<char *>(SDL_malloc(totalMemorySize))
    };

    int width, height;
    SDL_GetWindowSize(statePtr->window, &width, &height);
    Clay_Initialize(
            clayMemory,
            (Clay_Dimensions)
                {(float) width, (float) height},
                (Clay_ErrorHandler) {HandleClayErrors});
    Clay_SetMeasureTextFunction(SDL_MeasureText, statePtr->rendererData.fonts);

    statePtr->simulation = std::make_unique<Simulation>(SDL_Rect {200, 0, width - 200, height - 0}, 1000, 10);
    *appstate = statePtr;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    const auto *statePtr = static_cast<AppState*>(appstate);
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
            Clay_SetPointerState((Clay_Vector2) {event->motion.x, event->motion.y}, event->motion.state & SDL_BUTTON_LMASK);
        break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            Clay_SetPointerState((Clay_Vector2) {event->button.x, event->button.y}, event->button.button == SDL_BUTTON_LEFT);
            if(event->button.button == SDL_BUTTON_RIGHT)
                statePtr->simulation->setUserAction(UserActionType::NONE, UIData{});
        }
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
    auto *statePtr = static_cast<AppState*>(appstate);
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

    if(timeAccumForFixedUpdate >= 0.016f) {
        statePtr->simulation->fixedUpdate();
        timeAccumForFixedUpdate = 0.0f;
    }else timeAccumForFixedUpdate += deltaTime;

    int width, height;
    SDL_GetWindowSize(statePtr->window, &width, &height);

    statePtr->simulation->update(SDL_Rect {200, 0, width - 200, height - 0}, deltaTime);
    std::stringstream quadSizeStream;
    quadSizeStream << "QuadTree Size: " << statePtr->simulation->getQuadSize();
    Clay_RenderCommandArray renderCommands = Clay_CreateLayout(width, height, statePtr->simulation, FPS, quadSizeStream.str());

    SDL_SetRenderDrawColor(statePtr->renderer, 255, 255, 255, 255);
    SDL_RenderClear(statePtr->renderer);

    statePtr->simulation->render(statePtr->renderer);
    SDL_Clay_RenderClayCommands(&statePtr->rendererData, &renderCommands);

    SDL_RenderPresent(statePtr->renderer);

    //#ifndef NDEBUG
    //    int debugWidth, debugHeight;
    //    SDL_GetWindowSize(statePtr->debugWindow, &debugWidth, &debugHeight);
    //    Clay_RenderCommandArray debugRenderCommands = Clay_Debug_CreateLayout(debugWidth, debugHeight, FPS);
    //    SDL_SetRenderDrawColor(statePtr->debugRenderer, 255, 255, 255, 255);
    //    SDL_RenderClear(statePtr->debugRenderer);
    //    SDL_Clay_RenderClayCommands(&statePtr->debugRendererData, &debugRenderCommands);
    //    SDL_RenderPresent(statePtr->debugRenderer);
    //#endif

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    //(void) result

    if(result != SDL_APP_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Application failed to run");
    }

    auto *statePtr = static_cast<AppState*>(appstate);

    if(statePtr) {
    //renderer is already freed?
        if(statePtr->renderer)
            SDL_DestroyRenderer(statePtr->renderer);

        if(statePtr->debugRenderer)
            SDL_DestroyRenderer(statePtr->debugRenderer);

        if(statePtr->window)
            SDL_DestroyWindow(statePtr->window);

        if(statePtr->debugWindow)
            SDL_DestroyWindow(statePtr->debugWindow);

        if(statePtr->rendererData.fonts) {
            for(size_t i = 0; i < sizeof(statePtr->rendererData.fonts) / sizeof(*statePtr->rendererData.fonts); i++) {
                TTF_CloseFont(statePtr->rendererData.fonts[i]);
            }
        }
        SDL_free(statePtr->rendererData.fonts);
        SDL_free(statePtr);
    }
    TTF_Quit();
    SDL_Quit();
}

