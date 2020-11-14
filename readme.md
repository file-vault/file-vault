## 运行方式

### How to run

#### Reqirements

- mysql  -uroot -p123456
- sudo apt-get install openssl-devel (this is for openssl.h)
- sudo apt-get install libmysql++-dev (this is for mysql.h)
- sudo apt install e2fslibs-dev
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
  ./build.sh
  # this is for users registration
  user1$ ./vault-cli -a /home/user1/TestAudit
  user2$ ./vault-cli -a /home/user2/TestAudit
  ./vault-cli -l
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

- Now, do the experiment, to read/write/open the files in TestAudit (the protected dir). 

  ```shell
  user2$ cat /home/user1/TestAudit/hhh.txt
  # Permission denied.
  ```

- Problem: when killing **daemon** process, the system broke down.

## 项目中期检查报告

## 项目名称：基于系统调用重载的基本型文件保险箱
 
### 一、	项目开发目标
用户需要为重要数据提供严格的安全保障，因此设计一个专属于用户的文件夹保险箱用于数据存储，同时开发一个保险箱管理程序对文件保险箱内的数据进行增删查，并通过系统调用重载保证该文件夹不会被其它人查看、访问、修改以及删除。希望该程序具有较好的用户体验，满足用户各种操作需求和使用习惯。并在安全审计、加解密两方面进行安全加固。

### 二、	项目开发内容
1. 满足用户的各种操作需求，包括增删查保险箱内的相关文件（文件信息存放于数据库）。具体工作包括：

1）设计一个程序：对于每一个用户，给定指令+文件/文件夹路径，可以将其inode节点号从该用户对应的表格(t1000 t1001等，其中1000 1001分别表示该用户的uid)中插入或删除。此外可以查看该用户当前表格中的文件/文件夹有哪些（包括根据inode节点获得文件/文件夹路径）

2）配置数据库安全策略，使得每个用户只能增删自己的表格，但是可以查看所有表格（此举是为了保证守护进程能正常查询）

2. 内核态需要和用户态通信，从而获得指定用户是否具有查看某文件的权限。具体工作包括：

1)设计一个用户态的守护程序，和内核态以netlink形式通信。包括用户态和内核态完成netlink初始化配置、收发信息相关函数设计、信号量同步获取操作结果。

2)设计查询数据库的程序接口，及特权查询逻辑（若返回结果为res（1表示有权限），对于内核发送的uid和ino，先查用户自己的表，若在表中，则res=1，再查除用户表以外的所有表，若ino在表中，则返回0，否则返回1）。

3. 通过系统调用重载保证该文件夹不会被其它人查看、访问、修改以及删除。具体工作包括：

1）设计Hook函数，函数主要逻辑为在原本的操作前先查询用户是否具有操作的权限，根据查询结果选择执行原有操作还是直接返回-1

2）设计可动态加装拆卸模块，过程包括获取sys_call_table、去除写保护、更改原位置的函数地址为hook函数的地址、增加写保护。此外，需要将netlink的init release 及相关函数嵌入模块初始化和模块解绑的函数中

4. 该文件夹保险箱需要对于非法操作的尝试予以记录，完成安全审计。具体工作包括在hook函数内将记录通过netlink发给守护进程2或者用户态进程，相应进程将结果记录在日志文件中

5. 使用加解密进行加固。包括用户使用密码进入用户管理程序，并由管理程序将密钥发往内核，内核重载函数读写文件之前先进行加解密；用户退出管理程序后，注销之前发往内核的密钥。

### 三、	项目进展情况
1）完成用户管理程序，即可对数据库的操作，增删查保险箱内的文件。

2）完成了守护程序，可以和内核态通信，通过查询数据库返回该用户是否有对该文件操作的权利，并重载了相应操作函数包括open，read，write等

3）基本完成了加解密函数的设计

4）基本完成了安全审计的功能，会记录下用户非法操作的形式、时间、用户、文件路径，共享netlink信道由内核发向守护进程。

### 四、	目前存在的问题及建议
1. 在之前的总体设计中存在两个考虑欠妥的地方，因而此阶段的工作做出了调整。

1）内核态不能直接查看文件数据库，需要在用户态设计一个守护进程与内核通信完成保险箱数据库中文件特权的查询。

2）在用户间不可见的功能要求下，用户管理程序只是操作数据库增删文件、查保险箱内的文件有哪些，而不是解析数据查看文件内容，所以加解密不能做在用户态而要做在内核态，不过这需要用户态和内核态密钥的传递。

2. 此外，本阶段遇到的问题及解决方法如下：

1）内核态如何获取用户是否有操作文件权利：共有三种方法，法一内核操作配置文件（但是对于不同用户难于方便管理），法二内核直接操作数据库（内核似乎不可以直接使用mysql.h），因而使用法三即直接用netlink让内核与用户态守护进程通信，由用户态查询结果并返回（查询逻辑需要设计一下，具体见上）

2）如何通信：鉴于内核态消息接收函数只能以回调的形式使用，所以需要将发送和接收逻辑分开，并用信号量和全局变量协调。具体过程为模块初始化函数中sem初值设置为0，而后hook函数为发送请求并down(&sem)，此时sem为0不能down所以休眠等待，这时接收函数收到查询结果，修改全局变量check_result并up(&sem)，此时hook函数可以获得有效查询结果继续后面的处理。

3）如何从系统调用参数中获得inode号:通过查看源码资料等方式，了解到可以在regs->di, (char*)regs->si获取文件名进而获取文件inode号

### 五、	下一阶段工作安排
1）完成更多的重载函数设计，包括查询、移动、创建软链接等
2）加解密函数下一步需要引入内核重载函数进行整合测试，以及实现密钥传输过程
3）安全审计部分与已经设计好的重载函数的整合