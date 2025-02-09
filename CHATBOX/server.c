#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 256
#define SERVER_PORT 8080

// Structure to hold client information
typedef struct {
    SOCKET socket;
    char username[32];
} Client;

Client clients[MAX_CLIENTS];
int clientCount = 0;

// Function to broadcast message to all clients except sender
void broadcastMessage(const char* message, int senderIndex) {
    for (int i = 0; i < clientCount; i++) {
        if (i != senderIndex) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
}

// Function to remove client
void removeClient(int index) {
    closesocket(clients[index].socket);
    for (int i = index; i < clientCount - 1; i++) {
        clients[i] = clients[i + 1];
    }
    clientCount--;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
    
    // Create server socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return 1;
    }
    
    // Setup server address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);
    
    // Bind
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    // Listen
    if (listen(serverSocket, MAX_CLIENTS) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    
    printf("Server started on port %d\n", SERVER_PORT);
    
    fd_set readfds;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        
        // Add client sockets to fd_set
        for (int i = 0; i < clientCount; i++) {
            FD_SET(clients[i].socket, &readfds);
        }
        
        // Wait for activity on any socket
        if (select(0, &readfds, NULL, NULL, NULL) == SOCKET_ERROR) {
            printf("Select failed\n");
            break;
        }
        
        // Check for new connections
        if (FD_ISSET(serverSocket, &readfds)) {
            struct sockaddr_in clientAddr;
            int addrLen = sizeof(clientAddr);
            SOCKET newSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
            
            if (newSocket != INVALID_SOCKET) {
                if (clientCount < MAX_CLIENTS) {
                    // Receive username
                    char username[32];
                    int bytesReceived = recv(newSocket, username, sizeof(username), 0);
                    if (bytesReceived > 0) {
                        username[bytesReceived] = '\0';
                        
                        // Add new client
                        clients[clientCount].socket = newSocket;
                        strcpy(clients[clientCount].username, username);
                        
                        // Announce new user
                        char announcement[MAX_MESSAGE_LENGTH];
                        snprintf(announcement, MAX_MESSAGE_LENGTH, "%s has joined the chat", username);
                        broadcastMessage(announcement, clientCount);
                        
                        clientCount++;
                        printf("New client connected: %s\n", username);
                    }
                } else {
                    // Server is full
                    const char* msg = "Server is full";
                    send(newSocket, msg, strlen(msg), 0);
                    closesocket(newSocket);
                }
            }
        }
        
        // Check for messages from clients
        for (int i = 0; i < clientCount; i++) {
            if (FD_ISSET(clients[i].socket, &readfds)) {
                char buffer[MAX_MESSAGE_LENGTH];
                int bytesReceived = recv(clients[i].socket, buffer, sizeof(buffer), 0);
                
                if (bytesReceived > 0) {
                    // Message received
                    buffer[bytesReceived] = '\0';
                    broadcastMessage(buffer, i);
                    printf("Message from %s: %s\n", clients[i].username, buffer);
                }
                else {
                    // Client disconnected
                    char announcement[MAX_MESSAGE_LENGTH];
                    snprintf(announcement, MAX_MESSAGE_LENGTH, "%s has left the chat", clients[i].username);
                    removeClient(i);
                    broadcastMessage(announcement, -1);
                    printf("Client disconnected: %s\n", announcement);
                    i--;
                }
            }
        }
    }
    
    // Cleanup
    closesocket(serverSocket);
    for (int i = 0; i < clientCount; i++) {
        closesocket(clients[i].socket);
    }
    WSACleanup();
    
    return 0;
}