#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_TEXT_LENGTH 256
#define MAX_MESSAGES 15
#define CHATBOX_HEIGHT 400
#define INPUT_BOX_HEIGHT 40

SOCKET clientSocket;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
TTF_Font *font = NULL;
char chatMessages[MAX_MESSAGES][MAX_TEXT_LENGTH];
int messageCount = 0;
char inputText[MAX_TEXT_LENGTH] = "";
char username[MAX_TEXT_LENGTH] = "";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
bool should_exit = false;
pthread_t recv_thread;

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

void drawChatInterface(SDL_Renderer *renderer) {
    SDL_Rect chatArea = {20, 20, WINDOW_WIDTH - 40, CHATBOX_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderFillRect(renderer, &chatArea);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &chatArea);

    SDL_Rect inputBox = {20, WINDOW_HEIGHT - INPUT_BOX_HEIGHT - 20, 
                        WINDOW_WIDTH - 40, INPUT_BOX_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &inputBox);
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderDrawRect(renderer, &inputBox);
}

bool getUsernameInput() {
    bool entering = true;
    SDL_Event event;
    char tempInput[MAX_TEXT_LENGTH] = "";
    
    while (entering && !should_exit) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        SDL_Rect inputBox = {WINDOW_WIDTH/4, WINDOW_HEIGHT/2 - 60, 
                            WINDOW_WIDTH/2, INPUT_BOX_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &inputBox);
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer, &inputBox);
        
        renderText(renderer, font, "Enter your username:", 
                  WINDOW_WIDTH/4, WINDOW_HEIGHT/2 - 100);
        renderText(renderer, font, tempInput, 
                  WINDOW_WIDTH/4 + 10, WINDOW_HEIGHT/2 - 50);
        SDL_RenderPresent(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                should_exit = true;
                return false;
            }
            
            if (event.type == SDL_TEXTINPUT) {
                if (strlen(tempInput) < MAX_TEXT_LENGTH - 1) {
                    strcat(tempInput, event.text.text);
                }
            }
            
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(tempInput) > 0) {
                    tempInput[strlen(tempInput) - 1] = '\0';
                }
                if (event.key.keysym.sym == SDLK_RETURN && strlen(tempInput) > 0) {
                    strcpy(username, tempInput);
                    send(clientSocket, username, strlen(username), 0);
                    return true;
                }
            }
        }
        SDL_Delay(16);
    }
    return false;
}

void *receiveMessages(void *arg) {
    char buffer[MAX_TEXT_LENGTH];
    while (!should_exit) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            pthread_mutex_lock(&mutex);
            if (messageCount < MAX_MESSAGES) {
                strcpy(chatMessages[messageCount++], buffer);
            } else {
                for (int i = 0; i < MAX_MESSAGES - 1; i++) {
                    strcpy(chatMessages[i], chatMessages[i + 1]);
                }
                strcpy(chatMessages[MAX_MESSAGES - 1], buffer);
            }
            pthread_mutex_unlock(&mutex);
        } else if (bytesReceived <= 0) {
            break;
        }
        SDL_Delay(10);
    }
    return NULL;
}

void cleanup() {
    should_exit = true;
    pthread_join(recv_thread, NULL);
    SDL_StopTextInput();
    closesocket(clientSocket);
    WSACleanup();
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSA Startup failed\n");
        return 1;
    }
    
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Connection failed\n");
        WSACleanup();
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL Init failed\n");
        return 1;
    }
    if (TTF_Init() != 0) {
        printf("TTF Init failed\n");
        SDL_Quit();
        return 1;
    }
    
    window = SDL_CreateWindow("Chat Client", SDL_WINDOWPOS_CENTERED, 
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 
                            SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    font = TTF_OpenFont("Arial.ttf", 20);
    
    if (!getUsernameInput()) {
        cleanup();
        return 1;
    }
    
    if (pthread_create(&recv_thread, NULL, receiveMessages, NULL) != 0) {
        printf("Failed to create receiver thread\n");
        cleanup();
        return 1;
    }
    
    bool running = true;
    SDL_Event event;
    SDL_StartTextInput();
    
    while (running && !should_exit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
            
            if (event.type == SDL_TEXTINPUT) {
                if (strlen(inputText) < MAX_TEXT_LENGTH - 1) {
                    strcat(inputText, event.text.text);
                }
            }
            
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText) > 0) {
                    inputText[strlen(inputText) - 1] = '\0';
                }
                if (event.key.keysym.sym == SDLK_RETURN && strlen(inputText) > 0) {
                    char fullMessage[MAX_TEXT_LENGTH];
                    snprintf(fullMessage, MAX_TEXT_LENGTH, "%s: %s", username, inputText);
                    send(clientSocket, fullMessage, strlen(fullMessage), 0);
                    inputText[0] = '\0';
                }
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        drawChatInterface(renderer);
        
        pthread_mutex_lock(&mutex);
        int y = 30;
        for (int i = 0; i < messageCount; i++) {
            renderText(renderer, font, chatMessages[i], 30, y);
            y += 25;
        }
        pthread_mutex_unlock(&mutex);
        
        renderText(renderer, font, inputText, 30, WINDOW_HEIGHT - INPUT_BOX_HEIGHT - 10);
        
        SDL_RenderPresent(renderer);
        
        SDL_Delay(16);
    }
    
    cleanup();
    return 0;
}