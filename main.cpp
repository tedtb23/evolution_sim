#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

#define CLAY_IMPLEMENTATION
#include "clay.h"

#include <filesystem>
#include <string>
#include <sstream>
#include <cstring>
#include <iomanip>
#include "renderer/SDL3/clay_renderer_SDL3.c"
#include "Simulation.hpp"
#include "UIStructs.hpp"

static constexpr int WINDOW_WIDTH = 1280;
static constexpr int WINDOW_HEIGHT = 720;

static constexpr Uint32 FONT_SMALL = 0;
static constexpr Uint32 FONT_MEDIUM = 1;
static constexpr Uint32 FONT_LARGE= 2;

static constexpr Clay_Color COLOR_BLACK     = (Clay_Color) {0, 0, 0, 255};
static constexpr Clay_Color COLOR_WHITE     = (Clay_Color) {255, 255, 255, 255};
static constexpr Clay_Color COLOR_GREY      = (Clay_Color) {43, 41, 51, 255};
static constexpr Clay_Color COLOR_BLUE      = (Clay_Color) {50, 90, 162, 255};
static constexpr Clay_Color COLOR_LIGHT     = (Clay_Color) {224, 215, 210, 255};

struct ClayData {
    std::shared_ptr<Simulation> simPtr;
    SimData simData;
    std::string fpsStr;
    int windowWidth;
    int windowHeight;
    SDL_Surface* pauseImageDataPtr;
    SDL_Surface* playImageDataPtr;
};

struct AppState {
    SDL_Window* windowPtr;
    SDL_Renderer* rendererPtr;
    Clay_SDL3RendererData rendererData;
    std::shared_ptr<Simulation> simPtr;
    ClayData clayData;
};

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
        const auto clayDataPtr = reinterpret_cast<ClayData*>(userData);
        const auto& simPtr = clayDataPtr->simPtr;
        if(strcmp(elementID.stringId.chars, "Button_Add_Food") == 0) {
            simPtr->setUserAction(UserActionType::ADD_FOOD, UIData{});
        }else if(strcmp(elementID.stringId.chars, "Button_Show_QuadTree") == 0) {
            static bool quadIsShown = false;
            quadIsShown = !quadIsShown;
            simPtr->showQuadTree(quadIsShown);
        }else if(strcmp(elementID.stringId.chars, "Button_Pause") == 0) {
            static bool paused = false;
            paused = !paused;
            if(paused)
                simPtr->setUserAction(UserActionType::PAUSE, UIData{});
            else
                simPtr->setUserAction(UserActionType::UNPAUSE, UIData{});
        }else if(strcmp(elementID.stringId.chars, "Button_Close_Organism_ToolTip") == 0) {
            clayDataPtr->simPtr->setUserAction(UserActionType::UNFOCUS, {});
            clayDataPtr->simData = {};
        }else if(strcmp(elementID.stringId.chars, "Background") == 0) {
            clayDataPtr->simData.simObjectData = simPtr->userClicked(pointerData.position.x, pointerData.position.y);
            const OrganismData* organismDataPtr = std::get_if<OrganismData>(&clayDataPtr->simData.simObjectData);
            if(organismDataPtr) simPtr->setUserAction(UserActionType::FOCUS, organismDataPtr->id);
        }
    }
}

static Clay_RenderCommandArray Clay_CreateLayout(ClayData* dataPtr) {
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
        Clay_OnHover(handleButtonPress, reinterpret_cast<intptr_t>(dataPtr));
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
        .backgroundColor = COLOR_GREY,
        .scroll = {.vertical = true},

    }) {
            CLAY({
            .id = CLAY_ID("Row_1"),
            .layout = {
                .childGap = 10,
                .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                .layoutDirection = CLAY_LEFT_TO_RIGHT,
            },
            }) {
                CLAY({
                    .id = CLAY_ID("Button_Pause"),
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIXED(20), .height = CLAY_SIZING_FIXED(20)},
                    },
                     .image = {
                        .imageData = static_cast<void*>(dataPtr->pauseImageDataPtr),
                        .sourceDimensions = {
                                .width = 20.0f,
                                .height = 20.0f
                            }
                    },
                }) {
                    Clay_OnHover(handleButtonPress, reinterpret_cast<intptr_t>(dataPtr));
                }
                CLAY({
                    .id = CLAY_ID("FPS_Container"),
                    .layout = {
                        .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                    },
                    .backgroundColor = COLOR_LIGHT,
                })
                CLAY_TEXT(((Clay_String) {.length = static_cast<int32_t>(dataPtr->fpsStr.length()), .chars = dataPtr->fpsStr.c_str()}),
                    CLAY_TEXT_CONFIG({
                        .textColor = COLOR_BLACK,
                        .fontId = FONT_SMALL,
                        .fontSize = 0,
                    })
                );
            }
            CLAY({
                .id = CLAY_ID("Population_Container"),
                .layout = {
                    .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                    .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                },
                .backgroundColor = COLOR_LIGHT
            }) {
                CLAY_TEXT(((Clay_String) {.length = static_cast<int32_t>(dataPtr->simData.populationStr.length()), .chars = dataPtr->simData.populationStr.c_str()}),
                    CLAY_TEXT_CONFIG({
                        .textColor = COLOR_BLACK,
                        .fontId = FONT_SMALL,
                        .fontSize = 0,
                    })
                );
            }
            CLAY({
                .id = CLAY_ID("Generation_Container"),
                .layout = {
                   .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                   .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
                },
                .backgroundColor = COLOR_LIGHT
            }) {
                CLAY_TEXT(((Clay_String) {.length = static_cast<int32_t>(dataPtr->simData.generationStr.length()), .chars = dataPtr->simData.generationStr.c_str()}),
                    CLAY_TEXT_CONFIG({
                        .textColor = COLOR_BLACK,
                        .fontId = FONT_SMALL,
                        .fontSize = 0,
                    })
                );
            }
            CLAY({
                .id = CLAY_ID("Button_Add_Food"),
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
                Clay_OnHover(handleButtonPress, reinterpret_cast<intptr_t>(dataPtr));
                CLAY_TEXT(CLAY_STRING("Add Food"),
                          CLAY_TEXT_CONFIG({
                              .textColor = COLOR_BLACK,
                              .fontId = FONT_MEDIUM,
                              .fontSize = 0,
                              }));
            }
            CLAY({
                .id = CLAY_ID("Button_Show_QuadTree"),
                .layout = {
                    .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                    .childAlignment = {
                        .x = CLAY_ALIGN_X_CENTER,
                        .y = CLAY_ALIGN_Y_CENTER,
                }
                },
                .backgroundColor = Clay_Hovered() ?  COLOR_BLUE : COLOR_LIGHT,
            }) {
                Clay_OnHover(handleButtonPress, reinterpret_cast<intptr_t>(dataPtr));
                CLAY_TEXT(CLAY_STRING("Show_QuadTree"),
                    CLAY_TEXT_CONFIG({
                        .textColor = COLOR_BLACK,
                        .fontId = FONT_MEDIUM,
                        .fontSize = 0,
                        .wrapMode = CLAY_TEXT_WRAP_NONE,
                    }));
            }
            if(dataPtr->simPtr->quadTreeIsShown())
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
                    CLAY_TEXT(((Clay_String){.length = static_cast<int32_t>(dataPtr->simData.quadTreeSizeStr.length()), .chars = dataPtr->simData.quadTreeSizeStr.c_str()}),
                         CLAY_TEXT_CONFIG({
                             .textColor = COLOR_BLACK,
                             .fontId = FONT_SMALL,
                             .fontSize = 0,
                        })
                    );
                }
        }
        if(!std::get<OrganismData>(dataPtr->simData.simObjectData).organismInfoStr.empty())
            CLAY({
                .id = CLAY_ID("Organism_ToolTip"),
                .layout = {
                    .sizing = {
                        .width = CLAY_SIZING_FIT(250, 450),
                        .height = CLAY_SIZING_FIT(200, 600),
                    },
                    .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                },
                .backgroundColor = COLOR_LIGHT,
            }) {
                CLAY({
                    .id = CLAY_ID("Button_Close_Organism_ToolTip"),
                    .layout = {
                        .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                        .childAlignment = {
                            .x = CLAY_ALIGN_X_CENTER,
                            .y = CLAY_ALIGN_Y_CENTER,
                        }
                    },
                    .backgroundColor = Clay_Hovered() ?  COLOR_BLUE : COLOR_LIGHT,
                    .floating = {
                        .attachPoints = {
                            .element = CLAY_ATTACH_POINT_RIGHT_TOP,
                            .parent = CLAY_ATTACH_POINT_RIGHT_TOP
                        },
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                    },
                }) {
                    Clay_OnHover(handleButtonPress, reinterpret_cast<intptr_t>(dataPtr));
                    CLAY_TEXT(CLAY_STRING("X"),
                        CLAY_TEXT_CONFIG({
                            .textColor = COLOR_BLACK,
                            .fontId = FONT_MEDIUM,
                            .fontSize = 0,
                            .wrapMode = CLAY_TEXT_WRAP_NONE,
                    }));
                }
                CLAY({
                    .id = CLAY_ID("Organism_Info"),
                    .layout = {
                        .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                    },
                }) {
                    CLAY_TEXT(((Clay_String){
                        .length = static_cast<int32_t>(std::get<OrganismData>(dataPtr->simData.simObjectData).organismInfoStr.length()),
                        .chars = std::get<OrganismData>(dataPtr->simData.simObjectData).organismInfoStr.c_str()}),
                        CLAY_TEXT_CONFIG({
                            .textColor = COLOR_BLACK,
                            .fontId = FONT_SMALL,
                            .fontSize = 0,
                        })
                    );
                }
                CLAY({
                    .id = CLAY_ID("NeuralNet_Inputs"),
                    .layout = {
                        .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                    },
                }) {
                    CLAY_TEXT(((Clay_String){
                        .length = static_cast<int32_t>(std::get<OrganismData>(dataPtr->simData.simObjectData).neuralNetInputStr.length()),
                        .chars = std::get<OrganismData>(dataPtr->simData.simObjectData).neuralNetInputStr.c_str()}),
                        CLAY_TEXT_CONFIG({
                            .textColor = COLOR_BLACK,
                            .fontId = FONT_SMALL,
                            .fontSize = 0,
                        })
                    );
                }
                CLAY({
                    .id = CLAY_ID("NeuralNet_Outputs"),
                    .layout = {
                        .padding = {.left = 5, .right = 5, .top = 5, .bottom = 5},
                    },
                }) {
                    CLAY_TEXT(((Clay_String){
                            .length = static_cast<int32_t>(std::get<OrganismData>(dataPtr->simData.simObjectData).neuralNetOutputStr.length()),
                            .chars = std::get<OrganismData>(dataPtr->simData.simObjectData).neuralNetOutputStr.c_str()}),
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

void HandleClayErrors(Clay_ErrorData errorData) {
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

    auto statePtr = new AppState;
    if(!statePtr) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate state memory: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if(!SDL_CreateWindowAndRenderer("Evolution Simulation", WINDOW_WIDTH, WINDOW_HEIGHT, 0, &statePtr->windowPtr, &statePtr->rendererPtr)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create window and renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    statePtr->rendererData.renderer = statePtr->rendererPtr;
    SDL_SetWindowResizable(statePtr->windowPtr, true);
    SDL_SetWindowMinimumSize(statePtr->windowPtr, 640, 480);

    statePtr->rendererData.textEngine = TTF_CreateRendererTextEngine(statePtr->rendererPtr);
    if (!statePtr->rendererData.textEngine) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create text engine from renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    statePtr->rendererData.fonts = static_cast<TTF_Font **>(SDL_calloc(3, sizeof(TTF_Font *)));
    if (!statePtr->rendererData.fonts) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to allocate memory for the font array: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const char* basePath = SDL_GetBasePath();
    std::filesystem::path base(basePath);
    std::filesystem::path pauseImagePath = base / "../resources/images/button_pause.png";
    std::filesystem::path playImagePath = base / "../resources/images/button_play.png";
    std::filesystem::path fontPath = base / "../resources/arial.ttf";
    std::string fontPathStr = fontPath.lexically_normal().string();
    std::string pauseImagePathStr = pauseImagePath.lexically_normal().string();
    std::string playImagePathStr = playImagePath.lexically_normal().string();

    SDL_Surface* pauseImageSurface = IMG_Load(pauseImagePathStr.c_str());
    SDL_Surface* playImageSurface = IMG_Load(playImagePathStr.c_str());

    TTF_Font* fontSmall = TTF_OpenFont(fontPathStr.c_str(), 16);
    TTF_Font* fontMedium = TTF_OpenFont(fontPathStr.c_str(), 24);
    TTF_Font* fontLarge = TTF_OpenFont(fontPathStr.c_str(), 30);

    if(!fontSmall || !fontMedium || !fontLarge) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load font: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    statePtr->rendererData.fonts[FONT_SMALL] = fontSmall;
    statePtr->rendererData.fonts[FONT_MEDIUM] = fontMedium;
    statePtr->rendererData.fonts[FONT_LARGE] = fontLarge;

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena clayMemory = (Clay_Arena) {
        .capacity = totalMemorySize,
        .memory = static_cast<char *>(SDL_malloc(totalMemorySize))
    };

    int width, height;
    SDL_GetWindowSize(statePtr->windowPtr, &width, &height);
    Clay_Initialize(
            clayMemory,
            (Clay_Dimensions)
                {(float) width, (float) height},
                (Clay_ErrorHandler) {HandleClayErrors});
    Clay_SetMeasureTextFunction(SDL_MeasureText, statePtr->rendererData.fonts);

    statePtr->simPtr = std::make_shared<Simulation>(
            SDL_Rect {200, 0, width - 200, height - 0},
            1000,
            30,
            0.01f);
    statePtr->clayData = ClayData{
            statePtr->simPtr,
            {
                {},
                std::string("QuadTree Size: 0"),
                std::string("Population: 0"),
                },
            std::string("FPS: 0"),
            width,
            height,
            pauseImageSurface,
            playImageSurface};
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
            if(event->button.button == SDL_BUTTON_RIGHT) {
                if(statePtr->simPtr->getCurrentUserAction() == UserActionType::PAUSE) {
                    statePtr->simPtr->setUserAction(UserActionType::UNPAUSE, UIData{});
                }else {
                    statePtr->simPtr->setUserAction(UserActionType::NONE, UIData{});
                }
            }
        }
        break;
        case SDL_EVENT_MOUSE_WHEEL:
            Clay_UpdateScrollContainers(true, (Clay_Vector2) {event->wheel.x, event->wheel.y}, 0.01f);
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
    static float timeAccumForTextUpdate = 0.0f;
    std::stringstream fpsStream;
    std::stringstream quadSizeStream;
    std::stringstream populationStream;
    std::stringstream generationStream;

    if(timeAccumForFixedUpdate >= 0.016f) {
        statePtr->simPtr->fixedUpdate();
        timeAccumForFixedUpdate = 0.0f;
    }else timeAccumForFixedUpdate += deltaTime;

    int width, height;
    SDL_GetWindowSize(statePtr->windowPtr, &width, &height);

    statePtr->simPtr->update(SDL_Rect {200, 0, width - 200, height - 0}, deltaTime);

    if(timeAccumForTextUpdate >= 1.0f) {
        timeAccumForTextUpdate = 0.0f;

        fpsStream << "FPS: " << std::fixed << std::setprecision(2) << (1.0f / deltaTime);
        quadSizeStream << "QuadTree Size: " << statePtr->simPtr->getQuadSize();
        populationStream << "Population: " << statePtr->simPtr->getCurrentPopulation();
        generationStream << "Generation: " << statePtr->simPtr->getCurrentGeneration();

        statePtr->clayData.fpsStr =fpsStream.str();
        fpsStream.clear();
        statePtr->clayData.simData.quadTreeSizeStr = quadSizeStream.str();
        quadSizeStream.clear();
        statePtr->clayData.simData.populationStr = populationStream.str();
        populationStream.clear();
        statePtr->clayData.simData.generationStr = generationStream.str();
        generationStream.clear();
        statePtr->clayData.simData.simObjectData = statePtr->simPtr->getFocusedSimObjectData();
    }else timeAccumForTextUpdate += deltaTime;

    statePtr->clayData.windowWidth = width;
    statePtr->clayData.windowHeight = height;
    Clay_RenderCommandArray renderCommands = Clay_CreateLayout(&statePtr->clayData);

    SDL_SetRenderDrawColor(statePtr->rendererPtr, 255, 255, 255, 255);
    SDL_RenderClear(statePtr->rendererPtr);

    statePtr->simPtr->render(statePtr->rendererPtr);
    SDL_Clay_RenderClayCommands(&statePtr->rendererData, &renderCommands);

    SDL_RenderPresent(statePtr->rendererPtr);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    if(result != SDL_APP_SUCCESS) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Application failed to run");
    }

    auto *statePtr = static_cast<AppState*>(appstate);

    if(statePtr) {
        if(statePtr->rendererPtr)
            SDL_DestroyRenderer(statePtr->rendererPtr);

        if(statePtr->windowPtr)
            SDL_DestroyWindow(statePtr->windowPtr);

        if(statePtr->rendererData.fonts) {
            for(size_t i = 0; i < sizeof(statePtr->rendererData.fonts) / sizeof(*statePtr->rendererData.fonts); i++) {
                TTF_CloseFont(statePtr->rendererData.fonts[i]);
            }
        }
        SDL_free(statePtr->rendererData.fonts);

        if(statePtr->clayData.pauseImageDataPtr)
            SDL_DestroySurface(statePtr->clayData.pauseImageDataPtr);
        if(statePtr->clayData.playImageDataPtr)
            SDL_DestroySurface(statePtr->clayData.playImageDataPtr);

        delete statePtr;
    }
    TTF_Quit();
    SDL_Quit();
}

