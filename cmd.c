#include <unistd.h>
#include <getopt.h>
#include "utils.h"
#include "mysql.h"

int cmd_register() {
    uid_t id = getuid();
    char uid[10];
    snprintf(uid, 10, "%u", id);
    if (has_table(uid)) {
        fprintf(stderr, "You have been registered!\n");
        return -1;
    }

    char *password = NULL;
    do {
        password = getpass("Enter you password: ");
    } while (strlen(password) == 0);
    char *hashed_password = compute_MD5(password);
    char query[200];

    sprintf(query, "insert into users (uid, password) values(%u,\"%s\");", id, hashed_password);
    if (!execute_cud(query)) return -1;

    if (!create_user_table(id)) return -1;

    printf("Successfully registered!\n");
    return 0;
}

int cmd_auth() {
    uid_t uid = getuid();
    char *password = NULL;
    char *hashed_password = NULL;
    password = getpass("Enter you password: ");
    hashed_password = compute_MD5(password);
    if (!auth(uid, hashed_password)) {
        fprintf(stderr, "Authentication failed.\n");
        return -1;
    }
    printf("Authentication succeed.\n");
    return 0;
}

int cmd_remove() {
    ulong ino = get_ino(optarg);
    if (ino == 0) {
        fprintf(stderr, "Unknown file: %s\n", optarg);
        return -1;
    }
    printf("Removing %lu...\n", get_ino(optarg));
    uid_t uid = getuid();
    char query[100];
    sprintf(query, "delete from `%u` where ino=%lu;", uid, ino);
    if (!execute_cud(query)) {
        fprintf(stderr, "Failed to remove file %lu.\n", ino);
        return -1;
    }
    return 0;
}

int cmd_add() {
    ulong ino = get_ino(optarg);
    if (ino == 0) {
        fprintf(stderr, "Unknown file: %s\n", optarg);
        return -1;
    }
    uid_t uid = getuid();
    char query[100];
    sprintf(query, "insert into `%u` (ino) values(%lu);", uid, ino);
    if (!execute_cud(query)) {
        fprintf(stderr, "Failed to add file %lu.\n", ino);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (connect_to_mysql() == -1) {
        return -1;
    }


    int c;
    struct option long_options[] = {
            {"register", no_argument,       NULL, 'r'},
            {"cmd_auth", no_argument,       NULL, 'a'},
            {"remove",   required_argument, NULL, 'R'},
            {"add",      required_argument, NULL, 'A'},
    };

    while ((c = getopt_long(argc, argv, "arA:R:", long_options, NULL)) != -1) {
        switch (c) {
            case 'r': {
                //register
                cmd_register();
                break;
            }
            case 'a': {
                //auth
                cmd_auth();
                break;
            }
            case 'R': {
                //remove
                cmd_remove();
                break;
            }
            case 'A': {
                //add
                cmd_add();
                break;
            }
            default: {
                close_connection();
                exit(EXIT_FAILURE);
            }
        }
    }

    close_connection();
    return 0;
}