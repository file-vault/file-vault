#include <unistd.h>
#include <getopt.h>
#include "utils.h"
#include "mysql.h"

int cmd_register() {
    if (has_registered()) {
        fprintf(stderr, "You have been registered!\n");
        return -1;
    }

    char *password = NULL;
    do {
        password = getpass("Enter you password: ");
    } while (strlen(password) == 0);
    char *hashed_password = compute_MD5(password);

    if (!create_user(hashed_password)) return -1;

    if (!create_user_table()) return -1;

    printf("Successfully registered!\n");
    return 0;
}

int cmd_auth() {
    char *password = NULL;
    char *hashed_password = NULL;
    password = getpass("Enter you password: ");
    hashed_password = compute_MD5(password);
    if (!auth(hashed_password)) {
        free(hashed_password);
        fprintf(stderr, "Authentication failed.\n");
        return -1;
    }
    free(hashed_password);
    printf("Authentication succeed.\n");
    return 0;
}

int cmd_remove() {
    ino_t ino = get_ino(optarg);
    if (ino == 0) {
        fprintf(stderr, "Unknown file: %s\n", optarg);
        return -1;
    }
    if (!remove_file(ino)) return -1;
    return 0;
}

int cmd_add() {
    ino_t ino = get_ino(optarg);
    if (ino == 0) {
        fprintf(stderr, "Unknown file: %s\n", optarg);
        return -1;
    }
    if (!add_file(ino)) return -1;
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