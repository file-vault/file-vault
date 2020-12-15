#include "mysql.h"

int connect_to_mysql() {
    if (mysql_library_init(0, NULL, NULL)) {
        fprintf(stderr, "Unable to initialize MySQL client library.\n");
        return -1;
    }

    mysql = mysql_init(NULL);

    if (!mysql_real_connect(mysql, NULL, user, passwd, db, 3306, NULL, 0)) {
        fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(mysql));
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
    const char *format = "select * from users where uid=%u;";
    char *query = (char *) malloc((strlen(format) + get_unsigned_length(uid)) * sizeof(char));
    sprintf(query, format, uid);
    if (mysql_query(mysql, query)) {
        fprintf(stderr, "Query \"%s\" failed: %s\n", query, mysql_error(mysql));
        return false;
    }
    free(query);
    MYSQL_RES *res = mysql_store_result(mysql);
    if (!mysql_num_rows(res)) return false;
    mysql_free_result(res);
    return true;
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

bool add_ino(uid_t uid,ino_t ino) {
    static const char *format = "insert into info (uid,ino) values(%lu,%u);";
    char *query = (char *) malloc(
            (strlen(format) + get_unsigned_length(uid) + get_unsigned_length(ino)) * sizeof(char));
    sprintf(query, format, uid, ino);
    if (!execute_cud(query)) {
        free(query);
        return false;
    }
    free(query);
    return true;
}

bool remove_ino(uid_t uid,ino_t ino) {
    const char *format = "delete from info where uid=%u and ino=%lu;";
    char *query = (char *) malloc(
            (strlen(format) + get_unsigned_length(uid) + get_unsigned_length(ino)) * sizeof(char));
    sprintf(query, format, uid, ino);
    if (!execute_cud(query)) {
        free(query);
        return false;
    }
    free(query);
    return true;
}

bool remove_file(uid_t uid,const char *path) {
    ino_t ino = get_ino(path);
    if (ino == 0) {
        fprintf(stderr, "Unknown file: %s\n", optarg);
        return false;
    }
    if (!remove_ino(uid,ino)) {
        fprintf(stderr, "Failed to remove file %lu.\n", ino);
        return false;
    }
    if (is_dir(path)) {
        walk_dir(uid,path, remove_ino);
        printf("Directory %lu removed.\n", ino);
    } else {
        printf("File %lu removed.\n", ino);
    }
    return true;
}

bool add_file(uid_t uid,const char *path) {
    ino_t ino = get_ino(path);
    if (ino == 0) {
        fprintf(stderr, "Unknown file: %s\n", optarg);
        return false;
    }
    if (ino == -1) {
        fprintf(stderr, "You are not the owner of the file: %s\n", optarg);
        return false;
    }
    if (!add_ino(uid,ino)) {
        fprintf(stderr, "Failed to add file %lu.\n", ino);
        return false;
    }
    if (is_dir(path)) {
        walk_dir(uid,path, add_ino);
        printf("Directory %lu added.\n", ino);
    } else {
        printf("File %lu added.\n", ino);
    }
    return true;
}

void walk_dir(uid_t uid,const char *path, callback c) {
    struct dirent *entry;

    DIR *dir_p = opendir(path);
    while ((entry = readdir(dir_p)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if ((entry->d_type & DT_DIR) == DT_DIR) {
            char *sub_path = (char *) malloc((strlen(path) + 1 + strlen(entry->d_name)) * sizeof(char));
            sprintf(sub_path, "%s/%s", path, entry->d_name);
            walk_dir(uid,sub_path, c);
            free(sub_path);
        }
        c(uid,entry->d_ino);
    }
}

bool delete_user() {
    uid_t uid = getuid();
    const char *format = "delete from users where uid=%u;";
    char *query = (char *) malloc((strlen(format) + get_unsigned_length(uid)) * sizeof(char));
    sprintf(query, format, uid);
    if (!execute_cud(query)) {
        free(query);
        return false;
    }
    free(query);
    return true;
}

bool delete_user_data() {
    uid_t uid = getuid();
    const char *format = "delete from info where uid=%u;";
    char *query = (char *) malloc((strlen(format) + get_unsigned_length(uid) * sizeof(char)));
    sprintf(query, format, uid);
    if (!execute_cud(query)) {
        free(query);
        return false;
    }
    free(query);
    return true;
}

int fetch_inodes(uid_t uid,ext2_ino_t *files[]) {
    const char *format = "select ino from info where uid=%u;";
    char *query = (char *) malloc((strlen(format) + get_unsigned_length(uid)) * sizeof(char));
    sprintf(query, format, uid);
    if (mysql_query(mysql, query)) {
        fprintf(stderr, "Query \"%s\" failed: %s\n", query, mysql_error(mysql));
        return 0;
    }
    MYSQL_RES *res = mysql_store_result(mysql);
    int rows = mysql_num_rows(res);
    *files = (ext2_ino_t *) malloc(rows * sizeof(ext2_ino_t));
    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(res))) {
        (*files)[i] = (ext2_ino_t) strtol(row[0], NULL, 10);
        i++;
    }
    mysql_free_result(res);
    return rows;
}

int get_uids(uid_t *uids[]) {
    const char *query = "select * from users";
    if (mysql_query(mysql, query)) {
        fprintf(stderr, "Query \"%s\" failed: %s\n", query, mysql_error(mysql));
        return 0;
    }
    MYSQL_RES *res = mysql_store_result(mysql);
    int rows = mysql_num_rows(res);
    (*uids) = (uid_t *) malloc(rows * sizeof(ext2_ino_t));
    MYSQL_ROW row;
    int i = 0;
    while ((row = mysql_fetch_row(res))) {
        (*uids)[i] = (uid_t) strtol(row[0], NULL, 10);
        i++;
    }
    mysql_free_result(res);
    return rows;
}