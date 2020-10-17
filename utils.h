
#ifndef FILE_VAULT_UTILS_H
#define FILE_VAULT_UTILS_H

#include <sys/stat.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Get st_ino of a file, the path argument points to the file. Upon failure, it shall return 0.
ino_t get_ino(const char *filepath);

// Compute MD5 of text, must call free() after use.
char *compute_MD5(const char *text);

// Get the length of an unsigned number.
unsigned get_unsigned_length(unsigned long num);

#endif //FILE_VAULT_UTILS_H
