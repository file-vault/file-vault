#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include "utils.h"
#include "mysql.h"

int register_user() {
    char *password = NULL;
    do {
        password = getpass("\033[33mEnter you password: \033[0m");
    } while (strlen(password) == 0);
    char *hashed_password = compute_MD5(password);

    if (!create_user(hashed_password)) return -1;

    printf("\033[32mSuccessfully registered!\033[0m\n");
    return 0;
}

int auth_user() {
    char *password = NULL;
    char *hashed_password = NULL;
    password = getpass("\033[33mEnter you password: \033[0m");
    hashed_password = compute_MD5(password);
    if (!auth(hashed_password)) {
        free(hashed_password);
        fprintf(stderr, "\033[31mAuthentication failed.\033[0m\n");
        return -1;
    }
    free(hashed_password);
    printf("\033[34mAuthentication succeed.\033[0m\n");
    return 0;
}

int cmd_remove() {
    if (!remove_file(optarg)) return -1;
    return 0;
}

int cmd_add() {
    if (!add_file(optarg)) return -1;
    return 0;
}

void interactive() {
    char *action = NULL;
    size_t len = 0, num;
    do {
        printf("\033[32m>>\033[0m");
        if ((num = getline(&action, &len, stdin)) <= 0)break;
        if (action[num - 1] == '\n')action[num - 1] = '\0';
        if (strncasecmp(action, "ls", 2) == 0) {
            //ls
        } else if (strncasecmp(action, "remove", 2) == 0) {
            //remove from vault
        } else if (strncasecmp(action, "cd", 2) == 0) {
            //cd
        }
    } while (strcasecmp(action, "q") != 0); //Enter `Q` or `q` to exit.
    printf("Bye~\n");
    free(action);
}

int stop_vault() {
    if (!delete_user()) return -1;

    if (!delete_user_data()) return -1;
    printf("\033[32mFile vault stopped!\033[0m\n");
    return 0;
}

int list() {
    ext2_ino_t *files = NULL;
    int len = fetch_inodes(&files);
    print_filenames(files, len);
    free(files);
    return 0;
}

int main(int argc, char **argv) {
    atexit(close_connection); // Close MySQL connection before exit.

    if (connect_to_mysql() == -1) {
        exit(EXIT_FAILURE);
    }

    if (has_registered()) {
        if (auth_user() == -1) exit(EXIT_FAILURE);
    } else {
        if (register_user() == -1) exit(EXIT_FAILURE);
    }

    int c;
    struct option long_options[] = {
            {"remove",   required_argument, NULL, 'r'},
            {"add",      required_argument, NULL, 'a'},
            {"interact", no_argument,       NULL, 'i'},
//            {"enable",   no_argument,       NULL, 'e'},
//            {"disable",  no_argument,       NULL, 'd'},
            {"stop",     no_argument,       NULL, 's'},
            {"list",     no_argument,       NULL, 'l'},
    };

    while ((c = getopt_long(argc, argv, "deilsa:r:", long_options, NULL)) != -1) {
        switch (c) {
            case 'r': {
                //remove file or directory
                cmd_remove();
                break;
            }
            case 'a': {
                //add file or directory
                cmd_add();
                break;
            }
            case 'i': {
                interactive();
                break;
            }
            case 's': {
                stop_vault();
                break;
            }
            case 'l': {
                list();
                break;
            }
            default: {
                exit(EXIT_FAILURE);
            }
        }
    }
    exit(EXIT_SUCCESS);
}