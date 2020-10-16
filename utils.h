
#ifndef FILE_VAULT_UTILS_H
#define FILE_VAULT_UTILS_H

#include <sys/stat.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Get st_ino of a file, the path argument points to the file. Upon failure, it shall return 0.
unsigned long get_ino(const char *filepath);

//compute MD5 of text.
char *compute_MD5(const char *text);

#endif //FILE_VAULT_UTILS_H
