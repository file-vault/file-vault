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

bool has_registered() {
    uid_t uid = getuid();
    unsigned length = get_unsigned_length(uid);
    char *table = (char *) malloc((length + 1) * sizeof(char));
    sprintf(table, "%u", uid);
    MYSQL_RES *result = mysql_list_tables(mysql, NULL);
    if (result) {
        MYSQL_ROW row = mysql_fetch_row(result);
        while (row && strcmp(row[0], table) != 0) row = mysql_fetch_row(result);
        if (!row) {
            goto fail;
        }
        free(table);
        return true;
    }
    fail:
    free(table);
    return false;
}

bool create_user_table() {
    uid_t uid = getuid();
    const char *format = "create table if not exists `%u` (ino int unsigned not null primary key, dir bool not null);";
    char *query = (char *) malloc((strlen(format) + get_unsigned_length(uid)) * sizeof(char));
    sprintf(query, format, uid);

    if (!mysql_query(mysql, query)) {
        free(query);
        return true;
    }
    fprintf(stderr, "Failed to create table %u: %s\n", uid, mysql_error(mysql));
    free(query);
    return false;
}

bool execute_cud(const char *query) {
    if (mysql_query(mysql, query)) {
        fprintf(stderr, "Query \"%s\" failed: %s\n", query, mysql_error(mysql));
        return false;
    }
    return true;
}

bool auth(const char *hash) {
    uid_t uid = getuid();
    const char *format = "select * from users where uid=%u and password=\"%s\";";
    char *query = (char *) malloc((strlen(format) + get_unsigned_length(uid) + strlen(hash)) * sizeof(char));
    sprintf(query, format, uid, hash);
    if (mysql_query(mysql, query)) {
        fprintf(stderr, "Query \"%s\" failed: %s\n", query, mysql_error(mysql));
        free(query);
        return false;
    }
    free(query);
    MYSQL_RES *result = mysql_store_result(mysql);
    if (!(result && mysql_num_rows(result))) return false;
    mysql_free_result(result);
    return true;
}

bool create_user(const char *hashed_password) {
    uid_t uid = getuid();
    const char *format = "insert into users (uid, password) values(%u,\"%s\");";
    char *query = (char *) malloc((strlen(format) + get_unsigned_length(uid) + strlen(hashed_password)) * sizeof(char));
    sprintf(query, format, uid, hashed_password);
    if (!execute_cud(query)) {
        free(query);
        return false;
    }
    free(query);
    return true;
}

bool remove_file(const char *path) {
    ino_t ino = get_ino(path);
    if (ino == 0) {
        fprintf(stderr, "Unknown file: %s\n", optarg);
        return false;
    }
    uid_t uid = getuid();
    const char *format = "delete from `%u` where ino=%lu;";
    char *query = (char *) malloc(
            (strlen(format) + get_unsigned_length(uid) + get_unsigned_length(ino)) * sizeof(char));
    sprintf(query, format, uid, ino);
    if (!execute_cud(query)) {
        free(query);
        fprintf(stderr, "Failed to remove file %lu.\n", ino);
        return false;
    }
    free(query);
    printf("File %lu removed.\n", ino);
    return true;
}

bool add_file(const char *path) {
    ino_t ino = get_ino(path);
    if (ino == 0) {
        fprintf(stderr, "Unknown file: %s\n", optarg);
        return false;
    }
    uid_t uid = getuid();
    const char *format = "insert into `%u` (ino, dir) values(%lu,%u);";
    char *query = (char *) malloc(
            (strlen(format) + get_unsigned_length(uid) + get_unsigned_length(ino) + 1) * sizeof(char));
    sprintf(query, format, uid, ino, is_dir(path));
    if (!execute_cud(query)) {
        free(query);
        fprintf(stderr, "Failed to add file %lu.\n", ino);
        return false;
    }
    free(query);
    printf("File %lu added.\n", ino);
    return true;
}