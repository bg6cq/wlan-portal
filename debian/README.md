# Debian install 

## 0. debian system install
select Web Server

## 1. install gcc etc
````
apt-get install git gcc mysql-server mysql-client net-tools make libmysql++-dev libnetfilter-queue-dev libnl-nf-3-200 ipset screen apache2
````

## 2. get source & compile

maybe you need change the PH of redir_url.c

````
cd /usr/src/
git clone https://github.com/bg6cq/wlan-portal.git
cd wlan-portal
git checkout -b local
ln -s /usr/lib/cgi-bin /var/www
mkdir -p /var/lib/mysql
ln -s /var/run/mysqld/mysqld.sock /var/lib/mysql/mysql.sock
ln -s /sbin/ipset /usr/sbin
touch /var/www/cgi-bin/ip
make
````

## 4. install db
````
echo "create database wlan;" | mysql
mysql wlan < sql/wlan.sql
````

## 5. enable apache2 cgi
````
a2enmod cgi
systemctl restart apache2
````

## 6. install & config DNS/DHCP if need
````
apt-get install isc-dhcp-server bind9 bind9utils
````

## 7. /etc/rc.local

````
#!/bin/sh
#

SUBNET=172.16.80.0/23

#logined users
ipset create user bitmap:ip,mac range $SUBNET timeout 604800

#new users (could not do auto login)
ipset create newuser bitmap:ip,mac range $SUBNET timeout 604800

echo 1 > /proc/sys/net/ipv4/ip_forward

cd /usr/src/wlan-portal
screen -S wlan_redir -d -m ./run_redir
screen -S wlan_autologin -d -m ./run_autologin
````

enable rc.local
````
chmod a+x /etc/rc.local
systemctl start rc-local
````

## 8. /etc/network/if-pre-up.d/iptables
````
#!/bin/sh

iptables -t mangle -F
iptables -F

OUTIF=ens192

#set user was added by cgi program for logined user
iptables -t mangle -I PREROUTING -j MARK --set-mark 1 \
        -m set --match-set user src,src 
#set newuser was added by auto_login for newcome user
iptables -t mangle -I PREROUTING -j MARK --set-mark 2 \
        -m set --match-set newuser src,src 

iptables -A FORWARD -j ACCEPT -i $OUTIF

#allow logined user packets
iptables -A FORWARD -j ACCEPT -m mark --mark 1

#redirect unlogin user tcp traffic to redir_url via NFQUEUE 
iptables -A FORWARD -j NFQUEUE --queue-num 0 -p tcp --dport 80 -m limit --limit 10000/s

#drop packets from newuser, they must use redir
iptables -A FORWARD -j DROP -m mark --mark 2

#send packets to auto_login
iptables -A FORWARD -j NFQUEUE --queue-num 1

#drop unlogin user traffic
iptables -A FORWARD -j DROP

iptables -A INPUT -j ACCEPT -p tcp --dport 22 -s 172.16.21.0/24
iptables -A INPUT -j DROP -p tcp --dport 22
````

chmod a+x  /etc/network/if-pre-up.d/iptables 
