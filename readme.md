## 这是含bug的测试版本

I have no idea which step is wrong... or how to debug.



### How to run

#### Reqirements

- mysql  -uroot -p123456
- sudo apt-get install openssl-devel (this is for openssl.h)
- sudo apt-get install libmysql++-dev (this is for mysql.h)
- sudo snap install cmake --classic



#### Steps

- add user (suppose you are user1)

  ```shell
  sudo useradd -m user2
  sudo passwd user2
  ```

- mysql (open under ./cli)

  ```mysql
  create schema filevault
  use filevault
  # create users table 
  source users_table.sql
  ```

- ./cli

  ```shell
  sudo cmake ./
  sudo make
  # this is for users registration
  user1$ ./vault-cli -a /home/user1/TestAudit
  user2$ ./vault-cli -a /home/user2/TestAudit
  # now tables t1000 and t1001 has been built, with inode items added
  ```

- ./kernel

  ```shell
  # load module to kernel
  sudo make test
  # to check kernel log:
  make show
  # or 
  dmesg | tail -n 1000 | grep xxxx
  # where xxx refers to uid or inode number (or else to check)
  ```

- ./

  ```shell
  # this is daemon process, first compile .c
  gcc user_daemon_process.c -lmysqlclient -o user_daemon_process
  ./user_daemon_process
  ```

- Now, do the experiment, to read/write/open the files in TestAudit (the protected dir). What I have done is:

  ```shell
  user1$ vim /home/user1/TestAudit/hhh.txt
  # Killed
  user1$ cat /home/user2/TestAudit/test_file.txt
  # This is the content of test_file.
  ```

  The problem is: the program kill vim (for opening all files, even it's not in other's vault) , but we can still read from other's vault.