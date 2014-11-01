CFLAGS=-I/usr/include/mysql -L/usr/lib/mysql -Wall -L/usr/lib64/mysql


all:install redir_url

ip: ip.c cgilib.c
	gcc -g -o ip $(CFLAGS) ip.c  -lmysqlclient 

install:ip 
	rm /var/www/cgi-bin/ip
	cp ip /var/www/cgi-bin
	cp index.html /var/www/html
	cp wlan-portal.css /var/www/html
	cp mstage0.html /var/www/html/stage0.html
	cp mstage0.html /var/www/html
	cp mstage1.html /var/www/html/stage1.html
	cp mstage1.html /var/www/html
	cp mstage2.html /var/www/html/stage2.html
	cp mstage2.html /var/www/html
	cp help.html /var/www/html
	cp redir.html /var/www/html
	chown root /var/www/cgi-bin/ip
	chmod u+s /var/www/cgi-bin/ip

redir_url: redir_url.c
	gcc -Wall $(CFLAGS) -g -lnetfilter_queue $< -o $@

clean :
	rm -f redir_url

