CFLAGS=-I/usr/include/mysql -L/usr/lib/mysql -Wall -L/usr/lib64/mysql


all:install

ip: ip.c cgilib.c
	gcc -g -o ip $(CFLAGS) ip.c  -lmysqlclient 

install:ip 
	rm /var/www/cgi-bin/ip
	cp ip /var/www/cgi-bin
	cp index.html /var/www/html
	cp mstage0.html /var/www/html/stage0.html
	cp mstage0.html /var/www/html
	cp mstage1.html /var/www/html/stage1.html
	cp mstage1.html /var/www/html
	cp mstage2.html /var/www/html/stage2.html
	cp mstage2.html /var/www/html
	chown root /var/www/cgi-bin/ip
	chmod u+s /var/www/cgi-bin/ip
