#/bin/bash
if [ $UID -ne 0 ]
then
    	echo Non root user. Please run as root.
else 
    	echo Root user
	make
	sudo insmod AuditModule.ko
	cd ../cli
	sudo cmake .
	sudo make
	sudo chmod u+s vault-cli
	gcc daemon.c -o daemon -lmysqlclient
	gcc daemon_log.c -o daemon_log -lmysqlclient	
	./daemon & ./daemon_log
fi
