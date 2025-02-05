#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "credentials.h"

void get_input_timestamps(char *start_timestamp, char *end_timestamp) {
    printf("Do you want to enter a timestamp range? (yes/no): ");
    char choice[4];
    scanf("%s", choice);

    if (strcmp(choice, "yes") == 0) {
        // If the user wants to enter a start and end timestamp
        printf("Enter the start timestamp in format (YYYY-MM-DD HH:MM:SS): ");
        scanf("%s", start_timestamp);
        printf("Enter the end timestamp in format (YYYY-MM-DD HH:MM:SS): ");
        scanf("%s", end_timestamp);
    } else {
        // Use "ALL" to fetch all messages
        strcpy(start_timestamp, "ALL");
        strcpy(end_timestamp, "ALL");
    }
}

void get_input_sender_receiver(char *sender, char *receiver) {
    printf("Enter the sender's name: ");
    scanf("%s", sender);
    printf("Enter the receiver's name: ");
    scanf("%s", receiver);
}

void execute_query(MYSQL *conn, const char *start_timestamp, const char *end_timestamp, const char *sender, const char *receiver) {
    MYSQL_RES *res;
    MYSQL_ROW row;

    char query[512];

    if (strcmp(start_timestamp, "ALL") == 0 && strcmp(end_timestamp, "ALL") == 0) {
        // Fetch all messages for the sender and receiver if no timestamp is provided
        sprintf(query, "SELECT * FROM messages WHERE sender = '%s' AND receiver = '%s'", sender, receiver);
    } else if (strcmp(start_timestamp, "ALL") == 0) {
        // Fetch messages before the end timestamp
        sprintf(query, "SELECT * FROM messages WHERE sender = '%s' AND receiver = '%s' AND timestamp <= '%s'", sender, receiver, end_timestamp);
    } else if (strcmp(end_timestamp, "ALL") == 0) {
        // Fetch messages after the start timestamp
        sprintf(query, "SELECT * FROM messages WHERE sender = '%s' AND receiver = '%s' AND timestamp >= '%s'", sender, receiver, start_timestamp);
    } else {
        // Fetch messages between the start and end timestamps
        sprintf(query, "SELECT * FROM messages WHERE sender = '%s' AND receiver = '%s' AND timestamp BETWEEN '%s' AND '%s'", sender, receiver, start_timestamp, end_timestamp);
    }

    if (mysql_query(conn, query)) {
        fprintf(stderr, "QUERY ERROR: %s\n", mysql_error(conn));
        return;
    }

    res = mysql_store_result(conn);
    if (res == NULL) {
        fprintf(stderr, "ERROR: %s\n", mysql_error(conn));
        return;
    }

    printf("Messages from '%s' to '%s' within the specified timestamp range:\n", sender, receiver);
    while ((row = mysql_fetch_row(res))) {
        printf("ID: %s, Sender: %s, Receiver: %s, Message: %s, Timestamp: %s\n",
               row[0], row[1], row[2], row[3], row[4]);
    }

    mysql_free_result(res);
}

int main() {
    MYSQL *conn;
   
    if (user == NULL || password == NULL || database == NULL) {
        fprintf(stderr, "ERROR: Missing environment variables\n");
        return 1;
    }

    // User input for sender and receiver
    char sender[50], receiver[50];
    get_input_sender_receiver(sender, receiver);  // Get sender and receiver from the user
    
    // Initialize MySQL connection
    conn = mysql_init(NULL);
    if (conn == NULL) {
        printf( "mysql_init() failed\n");

        exit(1);
    }

    // Connect to the database
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        printf("mysql_real_connect() failed\n");
        mysql_close(conn);
        exit(1);
    }

    char start_timestamp[20];  // For storing the start timestamp
    char end_timestamp[20];    // For storing the end timestamp
    get_input_timestamps(start_timestamp, end_timestamp);  // Prompt for timestamp input

    execute_query(conn, start_timestamp, end_timestamp, sender, receiver);  // Execute query based on the inputs

    mysql_close(conn);
    return 0;
}
