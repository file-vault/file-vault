#ifndef FILE_VAULT_MYSQL_H
#define FILE_VAULT_MYSQL_H

#include <mysql/mysql.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "utils.h"

#define user "root"
#define passwd "123456"
#define db "filevault"

// use "static" to hide from other source files
static MYSQL *mysql;

// callback function used by walk_dir()
typedef bool (*callback)(ino_t);

//Connect to MySQL, returns 0 upon success, returns -1 upon failure.
int connect_to_mysql();

//Close connection with MySQL.
void close_connection();

//Check if this user has registered.
bool has_registered();

// Insert a new record into table `users`, returns true upon success, returns false upon failure.
bool create_user(const char *hashed_password);

//Create table for a user, returns true upon success, returns false upon failure.
bool create_user_table();

// delete current user from table `users`, returns true upon success, returns false upon failure.
bool delete_user();

//drop current user's table, returns true upon success, returns false upon failure.
bool drop_user_table();

//execute MySQL create/update/delete query, returns true upon success, returns false upon failure.
bool execute_cud(const char *query);

//Authentication, returns true upon success, returns false upon failure.
bool auth(const char *hashed_password);

// Add a file with the given ino_t into file-vault, returns true upon success, returns false upon failure.
bool add_ino(ino_t ino);

// Remove a file with the given ino_t from file-vault, returns true upon success, returns false upon failure.
bool remove_ino(ino_t ino);

//Remove a file from file-vault, returns true upon success, returns false upon failure.
bool remove_file(const char *filepath);

//Add a file into file-vault, returns true upon success, returns false upon failure.
bool add_file(const char *filepath);

// Add(remove) all files in the given directory into(from) file-vault recursively.
void walk_dir(const char *path, callback);

#endif //FILE_VAULT_MYSQL_H