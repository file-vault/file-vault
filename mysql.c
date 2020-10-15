#include "mysql.h"

int connect_to_mysql() {
    if (mysql_library_init(0, NULL, NULL)) {
        fprintf(stderr, "Unable to initialize MySQL client library.\n");
        return -1;
    }

    mysql = mysql_init(NULL);

    if (!mysql_real_connect(mysql, NULL, user, passwd, db, 3306, NULL, 0)) {
        fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(mysql));
        mysql_close(mysql);
        mysql_library_end();
        return -1;
    }
    return 0;
}

void close_connection() {
    mysql_close(mysql);
    mysql_library_end();
}

bool has_table(char *table) {
    MYSQL_RES *result = mysql_list_tables(mysql, NULL);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        while (row && strcmp(row[0], table) != 0) row = mysql_fetch_row(result);
        if (!row) {
            return false;
        }
        return true;
    }
    return false;
}
