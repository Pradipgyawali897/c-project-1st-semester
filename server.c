#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define MAX_CLIENTS 10
#define PORT 8080
#define BUFFER_SIZE 1024

SOCKET clients[MAX_CLIENTS];
int client_count = 0;

void broadcast_message(char* message, int sender_index) {
    for (int i = 0; i < client_count; i++) {
        if (i != sender_index) {
            send(clients[i], message, strlen(message), 0);
        }
    }
}

void handle_clients() {
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    
    while (1) {
        FD_ZERO(&readfds);
        for (int i = 0; i < client_count; i++) {
            FD_SET(clients[i], &readfds);
        }

        select(0, &readfds, NULL, NULL, NULL);

        for (int i = 0; i < client_count; i++) {
            if (FD_ISSET(clients[i], &readfds)) {
                int bytes_received = recv(clients[i], buffer, BUFFER_SIZE, 0);
                if (bytes_received <= 0) {
                    closesocket(clients[i]);
                    for (int j = i; j < client_count - 1; j++) {
                        clients[j] = clients[j + 1];
                    }
                    client_count--;
                    printf("Client disconnected. Remaining clients: %d\n", client_count);
                    i--;
                } else {
                    buffer[bytes_received] = '\0';
                    broadcast_message(buffer, i);
                }
            }
        }
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    WSAStartup(MAKEWORD(2,2), &wsa);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);
    printf("Server started on port %d\n", PORT);

    while (1) {
        SOCKET new_client = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_count < MAX_CLIENTS) {
            clients[client_count++] = new_client;
            printf("New client connected. Total clients: %d\n", client_count);
        } else {
            send(new_client, "Server full", 11, 0);
            closesocket(new_client);
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}