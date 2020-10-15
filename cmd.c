#include <unistd.h>
#include <getopt.h>
#include "utils.h"
#include "mysql.h"

int main(int argc, char **argv) {


    int c;
    struct option long_options[] = {
            {"register", no_argument,       NULL, 'r'},
            {"auth",     no_argument,       NULL, 'a'},
            {"remove",   required_argument, NULL, 'R'},
            {"add",      required_argument, NULL, 'A'},
    };

    while ((c = getopt_long(argc, argv, "arA:R:", long_options, NULL)) != -1) {
        switch (c) {
            case 'r': {
                //register
                printf("Registering...\nYour uid is %d.\n", getuid());
                if (connect_to_mysql() == -1) {
                    exit(EXIT_FAILURE);
                }
                char uid[10];
                sprintf(uid, "%u", getuid());
                if (has_table(uid)) {
                    fprintf(stderr, "You have benn registered!.\n");
                    exit(EXIT_FAILURE);
                }
                create_table(uid);
                //TODO: register
                close_connection();
                break;
            }
            case 'a': {
                //auth
                printf("Auth...\n");
                if (connect_to_mysql() == -1) {
                    exit(EXIT_FAILURE);
                }
                close_connection();
                //TODO: auth
                break;
            }
            case 'R': {
                //remove
                ulong ino = get_ino(optarg);
                if (ino == 0) {
                    fprintf(stderr, "Unknown file: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                printf("Removing %lu...\n", get_ino(optarg));
                //TODO: remove
                break;
            }
            case 'A': {
                //add
                ulong ino = get_ino(optarg);
                if (ino == 0) {
                    fprintf(stderr, "Unknown file: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                printf("Adding %lu...\n", ino);
                //TODO: add
                break;
            }
            default:
                exit(EXIT_FAILURE);
        }
    }
    return 0;
}