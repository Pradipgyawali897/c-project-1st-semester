#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <pthread.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_image.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_MESSAGES 100
#define MAX_MESSAGE_LENGTH 256
#define SERVER_PORT 8080
#define SERVER_IP "192.168.18.192"

// Global variables
SDL_Texture* backgroundTexture = NULL;
Mix_Chunk* messageSound = NULL;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
SOCKET clientSocket;
char username[32] = "";
char messages[MAX_MESSAGES][MAX_MESSAGE_LENGTH];
int messageCount = 0;
char inputBuffer[MAX_MESSAGE_LENGTH] = "";
pthread_t receiveThread;
pthread_mutex_t messageMutex = PTHREAD_MUTEX_INITIALIZER;
int running = 1;

// Function to add a message to the message array
void addMessage(const char* msg) {
    pthread_mutex_lock(&messageMutex);
    if (messageCount < MAX_MESSAGES) {
        strncpy(messages[messageCount], msg, MAX_MESSAGE_LENGTH - 1);
        messages[messageCount][MAX_MESSAGE_LENGTH - 1] = '\0';
        messageCount++;
    } else {
        // Shift messages up to make room for new message
        for (int i = 0; i < MAX_MESSAGES - 1; i++) {
            strcpy(messages[i], messages[i + 1]);
        }
        strncpy(messages[MAX_MESSAGES - 1], msg, MAX_MESSAGE_LENGTH - 1);
    }
    pthread_mutex_unlock(&messageMutex);
}

// Thread function to receive messages
void* receiveMessages(void* arg) {
    char buffer[MAX_MESSAGE_LENGTH];
    while (running) {
        int bytesReceived = recv(clientSocket, buffer, MAX_MESSAGE_LENGTH - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            addMessage(buffer);
        } else if (bytesReceived <= 0) {
            addMessage("Disconnected from server");
            running = 0;
            break;
        }
    }
    return NULL;
}

// Function to render text
void renderText(const char* text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect rect = {x, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

// Function to get username
int getUserName() {
    SDL_Event event;
    SDL_Color textColor = {255, 255, 255, 255};
    int done = 0;
    
    while (!done) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        renderText("Enter your username:", 50, 250, textColor);
        renderText(username, 50, 300, textColor);
        
        SDL_RenderPresent(renderer);
        
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return 0;
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN && strlen(username) > 0) {
                    done = 1;
                }
                else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(username) > 0) {
                    username[strlen(username) - 1] = '\0';
                }
            }
            else if (event.type == SDL_TEXTINPUT) {
                if (strlen(username) < 31) {
                    strcat(username, event.text.text);
                }
            }
        }
    }
    return 1;
}

int main(int argc, char* argv[]) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
    
    // Initialize SDL and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 512) < 0) {
        printf("SDL_mixer initialization failed: %s\n", Mix_GetError());
        return 1;
    }

// Load the sound effect
    messageSound = Mix_LoadWAV("message.wav");  // Make sure you have this file
    if (!messageSound) {
        printf("Failed to load sound effect: %s\n", Mix_GetError());
        return 1;
}

    
    if (TTF_Init() < 0) {
        printf("SDL_ttf initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Create window and renderer
    window = SDL_CreateWindow("Chat Client", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
    printf("SDL_image initialization failed: %s\n", IMG_GetError());
    return 1;
}

// Load background image
SDL_Surface* backgroundSurface = IMG_Load("background.jpg");  // Put your image path here
if (!backgroundSurface) {
    printf("Failed to load background image: %s\n", IMG_GetError());
    return 1;
}
backgroundTexture = SDL_CreateTextureFromSurface(renderer, backgroundSurface);
SDL_FreeSurface(backgroundSurface);
    
    // Load font
    font = TTF_OpenFont("C:\\Windows\\Fonts\\LSANSD.ttf", 15);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        return 1;
    }
    
    // Get username
    SDL_StartTextInput();
    if (!getUserName()) {
        running = 0;
    }
    
    // Create socket and connect to server
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Connection failed\n");
        running = 0;
    }
    
    // Send username to server
    send(clientSocket, username, strlen(username), 0);
    
    // Create receive thread
    pthread_create(&receiveThread, NULL, receiveMessages, NULL);
    
    // Main loop
    SDL_Event event;
    SDL_Color textColor = {255, 255, 255, 255};
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN && strlen(inputBuffer) > 0) {
                    char fullMessage[MAX_MESSAGE_LENGTH];
                    snprintf(fullMessage, MAX_MESSAGE_LENGTH, "%s: %s", username, inputBuffer);
                    send(clientSocket, fullMessage, strlen(fullMessage), 0);
                    addMessage(fullMessage);
                    Mix_PlayChannel(-1, messageSound, 0);
                    inputBuffer[0] = '\0';
                }
                else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputBuffer) > 0) {
                    inputBuffer[strlen(inputBuffer) - 1] = '\0';
                }
            }
            else if (event.type == SDL_TEXTINPUT) {
                if (strlen(inputBuffer) < MAX_MESSAGE_LENGTH - 1) {
                    strcat(inputBuffer, event.text.text);
                }
            }
        }
        
        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
        
        // Draw chat area
        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 28);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_Rect chatArea = {10, 10, WINDOW_WIDTH - 20, WINDOW_HEIGHT - 70};
        SDL_RenderFillRect(renderer, &chatArea);
        
        // Draw input area
        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 28);
        SDL_Rect inputArea = {10, WINDOW_HEIGHT - 50, WINDOW_WIDTH - 20, 40};
        SDL_RenderFillRect(renderer, &inputArea);
        
        // Render messages
        pthread_mutex_lock(&messageMutex);
        int y = 20;
        for (int i = 0; i < messageCount; i++) {
            renderText(messages[i], 20, y, textColor);
            y += 25;
        }
        pthread_mutex_unlock(&messageMutex);
        
        // Render input text
        renderText(inputBuffer, 20, WINDOW_HEIGHT - 40, textColor);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    // Add these lines before other cleanup code
    if (messageSound) {
        Mix_FreeChunk(messageSound);
    }
    Mix_CloseAudio();
    Mix_Quit();
    SDL_DestroyTexture(backgroundTexture);
    IMG_Quit();
    
    // Cleanup
    pthread_join(receiveThread, NULL);
    closesocket(clientSocket);
    WSACleanup();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}