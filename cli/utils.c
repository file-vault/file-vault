#include "utils.h"

ino_t get_ino(const char *path) {
    struct stat buf;
    if (stat(path, &buf) != 0) return 0;
    if (buf.st_uid != getuid()) return -1;
    return buf.st_ino;
}

bool is_dir(const char *path) {
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}

char *compute_MD5(const char *text) {
    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5, text, strlen(text));
    unsigned char hex[MD5_DIGEST_LENGTH];
    MD5_Final(hex, &md5);
    char *result = (char *) malloc((MD5_DIGEST_LENGTH * 2 + 1) * sizeof(char));
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(result + 2 * i, "%02X", hex[i]);
    }
    result[MD5_DIGEST_LENGTH * 2] = '\0';
    return result;
}

unsigned get_unsigned_length(unsigned long num) {
    unsigned length = 0;
    while (num) {
        num /= 10;
        length++;
    }
    return length;
}