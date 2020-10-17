#ifndef FILE_VAULT_MYSQL_H
#define FILE_VAULT_MYSQL_H

#include <mysql/mysql.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

#define user "root"
#define passwd "123456"
#define db "filevault"

// use "static" to hide from other source files
static MYSQL *mysql;

//Connect to MySQL, returns 0 upon success, returns -1 upon failure.
int connect_to_mysql();

//Close connection with MySQL.
void close_connection();

//Check if this user has registered.
bool has_registered();

//Create table for a user, returns true upon success, returns false upon failure.
bool create_user_table();

//execute MySQL create/update/delete query, returns true upon success, returns false upon failure.
bool execute_cud(const char *query);

//Authentication, returns true upon success, returns false upon failure.
bool auth(const char *hashed_password);

// Insert a new record into table `users`, returns true upon success, returns false upon failure.
bool create_user(const char *hashed_password);

//Remove a file from file-vault, returns true upon success, returns false upon failure.
bool remove_file(const char *filepath);

//Add a file into file-vault, returns true upon success, returns false upon failure.
bool add_file(const char *filepath);

#endif //FILE_VAULT_MYSQL_H