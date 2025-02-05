#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_TEXT_LENGTH 256
#define MAX_MESSAGES 10

// Function to render text on the screen
void renderText(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y) {
    if (text == NULL || text[0] == '\0') {
        return;
    }
    
    SDL_Color color = {255, 255, 255, 255};  // White color
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) {
        printf("Error creating surface: %s\n", TTF_GetError());
        return;
    }
    
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        printf("Error creating texture: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect dstRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Function to get username input
void getUsername(SDL_Renderer *renderer, TTF_Font *font, char *username) {
    bool entering = true;
    SDL_Event event;
    char inputText[MAX_TEXT_LENGTH] = "";

    while (entering) {
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render prompt
        renderText(renderer, font, "Enter your name:", 300, 200);
        renderText(renderer, font, inputText, 300, 250);

        SDL_RenderPresent(renderer);

        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                entering = false;
                exit(0);
            } else if (event.type == SDL_TEXTINPUT) {
                if (strlen(inputText) + strlen(event.text.text) < MAX_TEXT_LENGTH - 1) {
                    strcat(inputText, event.text.text);
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                    inputText[strlen(inputText) - 1] = '\0';
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    if (strlen(inputText) > 0) {
                        strcpy(username, inputText);
                        entering = false;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // Initialize SDL and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL Initialization Error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        printf("TTF Initialization Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create the window and renderer
    SDL_Window *window = SDL_CreateWindow("SDL Chat Box",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window Creation Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer Creation Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Load the font (Make sure Arial.ttf is in the same folder as the executable)
    TTF_Font *font = TTF_OpenFont("Arial.ttf", 24);
    if (!font) {
        printf("Font Loading Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Get the username
    char username[MAX_TEXT_LENGTH] = "";
    getUsername(renderer, font, username);

    // Chat log storage
    char chatMessages[MAX_MESSAGES][MAX_TEXT_LENGTH];
    int messageCount = 0;
    for (int i = 0; i < MAX_MESSAGES; i++) {
        chatMessages[i][0] = '\0';
    }
    
    // Buffer for current input text
    char inputText[MAX_TEXT_LENGTH] = "";

    bool running = true;
    SDL_Event event;

    // Enable text input
    SDL_StartTextInput();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_TEXTINPUT) {
                if (strlen(inputText) + strlen(event.text.text) < MAX_TEXT_LENGTH - 1) {
                    strcat(inputText, event.text.text);
                }
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                    inputText[strlen(inputText) - 1] = '\0';
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    if (strlen(inputText) > 0) {
                        char fullMessage[MAX_TEXT_LENGTH];
                        snprintf(fullMessage, MAX_TEXT_LENGTH, "%s: %s", username, inputText);

                        if (messageCount < MAX_MESSAGES) {
                            strcpy(chatMessages[messageCount], fullMessage);
                            messageCount++;
                        } else {
                            for (int i = 1; i < MAX_MESSAGES; i++) {
                                strcpy(chatMessages[i - 1], chatMessages[i]);
                            }
                            strcpy(chatMessages[MAX_MESSAGES - 1], fullMessage);
                        }
                        inputText[0] = '\0';
                    }
                }
            }
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render chat log
        int y = 20;
        for (int i = 0; i < messageCount; i++) {
            renderText(renderer, font, chatMessages[i], 20, y);
            y += 30;
        }

        // Draw input box
        SDL_Rect dialogRect = {20, WINDOW_HEIGHT - 70, WINDOW_WIDTH - 40, 50};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &dialogRect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &dialogRect);

        // Render input text
        renderText(renderer, font, inputText, dialogRect.x + 10, dialogRect.y + 10);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Cleanup
    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
