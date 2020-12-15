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
    if (!remove_file(getuid(), optarg)) return -1;
    return 0;
}

int cmd_add() {
    if (!add_file(getuid(), optarg)) return -1;
    return 0;
}

int stop_vault() {
    if (!delete_user()) return -1;

    if (!delete_user_data()) return -1;
    printf("\033[32mFile vault stopped!\033[0m\n");
    return 0;
}

int list(uid_t uid) {
    ext2_ino_t *files = NULL;
    int len = fetch_inodes(uid, &files);
    print_filenames(files, len);
    printf("\n");
    free(files);
    return 0;
}

int root_list() {
    ext2_ino_t *files = NULL;
    uid_t *uids = NULL;
    int n = get_uids(&uids);
    for (int i = 0; i < n; i++) {
        printf("\nuid:\t%u", uids[i]);
        int len = fetch_inodes(uids[i], &files);
        print_filenames(files, len);
        printf("\n");
    }
    return 0;
}

void interactive() {
    char *action = NULL;
    size_t len = 0, num;
    do {
        printf("\033[32m>>\033[0m");
        if ((num = getline(&action, &len, stdin)) <= 0)break;
        if (action[num - 1] == '\n')action[num - 1] = '\0';
        if (strncasecmp(action, "list", 4) == 0) {
            if (getuid() == 0) {
                char *uid = strtok(action, " ");
                uid = strtok(NULL, " ");
                if (uid == NULL) {
                    root_list();
                } else {
                    list(strtoul(uid, NULL, 10));
                }
            } else {
                list(getuid());
            }
        } else if (strncasecmp(action, "add ", 4) == 0) {
            char *path = strtok(action, " ");
            path = strtok(NULL, " ");
            if (getuid() == 0) {
                char *uid = strtok(NULL, " ");
                if (uid == NULL) {
                    add_file(getuid(), path);
                } else {
                    add_file(strtoul(uid, NULL, 10), path);
                }
            } else {
                add_file(getuid(), path);
            }
        } else if (strncasecmp(action, "remove ", 7) == 0) {
            char *path = strtok(action, " ");
            path = strtok(NULL, " ");
            if (getuid() == 0) {
                char *uid = strtok(NULL, " ");
                if (uid == NULL) {
                    remove_file(getuid(), path);
                } else {
                    remove_file(strtoul(uid, NULL, 10), path);
                }
            } else {
                remove_file(getuid(), path);
            }
        } else if (strncasecmp(action, "rm-ino ", 7) == 0) {
            char *ino = strtok(action, " ");
            ino = strtok(NULL, " ");
            if (getuid() == 0) {
                char *uid = strtok(NULL, " ");
                if (uid == NULL) {
                    if (remove_ino(getuid(), strtoul(ino, NULL, 0))) {
                        printf("%s removed.\n", ino);
                    }
                } else {
                    if (remove_ino(strtoul(uid, NULL, 10), strtoul(ino, NULL, 0))) {
                        printf("%s removed.\n", ino);
                    }
                }
            } else {
                if (remove_ino(getuid(), strtoul(ino, NULL, 0))) {
                    printf("%s removed.\n", ino);
                }
            }
        } else if (strncasecmp(action, "clear", 5) == 0) {
            delete_user_data();
            printf("data cleared!\n");
        } else if (strcasecmp(action, "q") == 0) {
            return;
        }
    } while (strcasecmp(action, "q") != 0); //Enter `Q` or `q` to exit.
    printf("Bye~\n");
    free(action);
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
                list(getuid());
                break;
            }
            default: {
                exit(EXIT_FAILURE);
            }
        }
    }
    exit(EXIT_SUCCESS);
}