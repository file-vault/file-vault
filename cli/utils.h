
#ifndef FILE_VAULT_UTILS_H
#define FILE_VAULT_UTILS_H

#include <sys/stat.h>
#include <openssl/md5.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/unistd.h>
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#include <ctype.h>
#include <sys/types.h>
#include <com_err.h>

struct walk_struct {
    ext2_ino_t dir;
    ext2_ino_t *inodes;
    int inodes_num;
    int left_num;
    char *parent_name;
};

static ext2_filsys fs;

// Get st_ino of a file, the path argument points to the file. If the file is not found, it returns 0, if the owner of the file is not current user, it returns -1.
ino_t get_ino(const char *filepath);

// Check if the file is a directory.
bool is_dir(const char *filepath);

// Compute MD5 of text, must call free() after use.
char *compute_MD5(const char *text);

// Get the length of an unsigned number.
unsigned get_unsigned_length(unsigned long num);

//Print filenames of given inode numbers.
bool print_filenames(ext2_ino_t *, int length);

#endif //FILE_VAULT_UTILS_H
