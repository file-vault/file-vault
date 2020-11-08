cmake_minimum_required(VERSION 3.17)
project(file-vault C)

set(CMAKE_C_STANDARD 11)

link_libraries(mysqlclient)
link_libraries(crypto)

add_executable(vault-cli vault-cli.c utils.c utils.h mysql.c mysql.h)
