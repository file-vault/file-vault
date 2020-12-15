#include "utils.h"

ino_t get_ino(const char *path) {
    struct stat buf;
    if (stat(path, &buf) != 0) return 0;
    uid_t uid = getuid();
    if (buf.st_uid != uid && uid != 0) return -1;
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

int iter_callback(struct ext2_dir_entry *dirent, int offset EXT2FS_ATTR((unused)), int blocksize EXT2FS_ATTR((unused)),
                  char *buf EXT2FS_ATTR((unused)), void *private) {
    struct walk_struct *ws = (struct walk_struct *) private;
    struct ext2_inode inode;
    if (strcmp(".", dirent->name) == 0 || strcmp("..", dirent->name) == 0)
        return 0;
    for (int i = 0; i < ws->inodes_num; i++) {
        if (ws->inodes[i] == dirent->inode) {
            ws->left_num--;
            if (!ws->parent_name) {
                ext2fs_get_pathname(fs, ws->dir, 0, &ws->parent_name);
            }
            ext2fs_read_inode(fs, dirent->inode, &inode);
            if (!LINUX_S_ISDIR(inode.i_mode))
                printf("%u\t%s/%.*s\n", ws->inodes[i],
                       ws->parent_name,
                       ext2fs_dirent_name_len(dirent),
                       dirent->name);
            else
                printf("%u\t\033[1;34m%s/%.*s\033[0m\n", ws->inodes[i],
                       ws->parent_name,
                       ext2fs_dirent_name_len(dirent),
                       dirent->name);
        }
    }
    return 0;
}

bool print_filenames(ext2_ino_t inodes[], int length) {
    if (!length) return false;
    errcode_t err;
    struct walk_struct ws;
    ws.inodes = inodes;
    ws.inodes_num = length;
    ws.left_num = length;
    err = ext2fs_open("/dev/sda1", 0, 0, 0, unix_io_manager, &fs);
    if (err) {
        com_err("print_filenames", err, "while open filesystem");
        return false;
    }
    printf("\n\033[1mInode\tPathname\033[0m\n");
    struct ext2_inode inode;
    ext2_ino_t ino;
    ext2_inode_scan scan = 0;
    ext2fs_open_inode_scan(fs, 0, &scan);
    do { err = ext2fs_get_next_inode(scan, &ino, &inode); }
    while (err == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE);
    while (ino) {
        if (!inode.i_links_count || inode.i_dtime || !LINUX_S_ISDIR(inode.i_mode)) {
            goto next;
        }
        ws.dir = ino;
        ws.parent_name = NULL;
        ext2fs_dir_iterate(fs, ino, 0, NULL, iter_callback, (void *) &ws);
        ext2fs_free_mem(&ws.parent_name);
        if (!ws.left_num)break;
        next:
        do { err = ext2fs_get_next_inode(scan, &ino, &inode); }
        while (err == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE);
    }
    ext2fs_close_inode_scan(scan);
    ext2fs_close(fs);
    return true;
}
