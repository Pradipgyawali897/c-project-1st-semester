#include <stdio.h>
#include <mysql/mysql.h>

int main() {
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    const char *server = "localhost";
    const char *user = "your_username";  // Replace with your MySQL username
    const char *password = "your_password";  // Replace with your MySQL password
    const char *database = "your_database";  // Replace with your database name

    conn = mysql_init(NULL);

    // Connect to the database
    if (mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        printf("Connected to the database successfully.\n");

        // Execute a query
        if (mysql_query(conn, "SELECT * FROM your_table")) {  // Replace 'your_table' with your table name
            fprintf(stderr, "Query failed: %s\n", mysql_error(conn));
            return 1;
        }

        res = mysql_store_result(conn);
        int num_fields = mysql_num_fields(res);

        // Print the result
        while ((row = mysql_fetch_row(res))) {
            for (int i = 0; i < num_fields; i++) {
                printf("%s\t", row[i] ? row[i] : "NULL");
            }
            printf("\n");
        }

        // Free the result
        mysql_free_result(res);
    } else {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
    }

    // Close the connection
    mysql_close(conn);
    return 0;
}
