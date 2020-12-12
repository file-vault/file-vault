### Version of 2.0
1. clean the code
2. add install.sh for simple install
3. remove some useless but abstract part in sync-process
4. root privilege for hook func
5. interactive mode of vault-cli

### How to run
1) mysql init & user create
mysql -uroot -p123456
source users_table.sql
sudo useradd -m user2
sudo passwd user2
2) bash install.sh as a root
3) run ./vault-cli -a/-r/-l/-i detail use to check source code

### Reqirements
sudo apt-get install libmysql++-dev (this is for mysql.h)
sudo apt-get install openssl (this is for openssl.h)
sudo apt-get install libssl-dev (this is for openssl.h) 
sudo apt install e2fslibs-dev
sudo snap install cmake --classic

### Next step to do
1) secure exit in a uninstall.sh
2) print part of audit part
3) root privilege for user-manage part 
4) encrypt and decrypt

