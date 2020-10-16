#include "mysql.h"

int connect_to_mysql() {
    if (mysql_library_init(0, NULL, NULL)) {
        fprintf(stderr, "Unable to initialize MySQL client library.\n");
        return -1;
    }

    mysql = mysql_init(NULL);

    if (!mysql_real_connect(mysql, NULL, user, passwd, db, 3306, NULL, 0)) {
        fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(mysql));
        close_connection();
        return -1;
    }
    return 0;
}

void close_connection() {
    mysql_close(mysql);
    mysql_library_end();
}

bool has_table(const char *table) {
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

bool create_user_table(uid_t uid) {
    char query[200];
    sprintf(query, "create table if not exists `%u` (ino int unsigned not null primary key);", uid);
    if (!mysql_query(mysql, query)) {
        return true;
    }
    fprintf(stderr, "Failed to create table %u: %s\n", uid, mysql_error(mysql));
    return false;
}

bool execute_cud(const char *query) {
    if (mysql_query(mysql, query)) {
        fprintf(stderr, "Query \"%s\" failed: %s\n", query, mysql_error(mysql));
        return false;
    }
    return true;
}

bool auth(uid_t uid, const char *hash) {
    char query[200];
    sprintf(query, "select * from users where uid=%u and password=\"%s\";", uid, hash);
    if (mysql_query(mysql, query)) {
        fprintf(stderr, "Query \"%s\" failed: %s\n", query, mysql_error(mysql));
        return false;
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    if (!(result && mysql_num_rows(result))) return false;
    mysql_free_result(result);
    return true;
}