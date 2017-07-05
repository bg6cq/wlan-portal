
#define _GNU_SOURCE
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <asm/byteorder.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <errno.h>
  
#define MAXLEN 2048

#define DEBUG 1


#include <mysql.h>
#include <sys/file.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <crypt.h>

#define AUTHDBUSER 		"root"
#define AUTHDBPASSWD 	""
#define AUTHDB     		"wlan"
#define AUTHDBHOST 		"localhost"
#define AUTHDBPORT 		3306
#define AUTHDBSOCKPATH 	"/var/lib/mysql/mysql.sock"


char PHONE [MAXLEN];
char MAC [MAXLEN];
char DeferMSG [MAXLEN];

MYSQL *mysql = NULL;

MYSQL * ConnectDB( void )
{
        if( (mysql = mysql_init(NULL)) == NULL ) {
                fprintf(stderr,"mysql_init error");
		exit(0);
	}
        if( mysql_real_connect(mysql, AUTHDBHOST, AUTHDBUSER, AUTHDBPASSWD,
                AUTHDB, AUTHDBPORT, AUTHDBSOCKPATH, 0) == NULL ) {
                fprintf(stderr,"mysql_connect error");
		exit(0);
	}
        return mysql;
}


/* 执行sql语句 */
MYSQL_RES * ExecSQL ( char *sql, int haveresult )
{
    MYSQL_RES * mysql_res;
    if( mysql_query(mysql,sql) ) 
        fprintf(stderr,"mysql_querying error");
    if( haveresult ) {
        if( (mysql_res = mysql_store_result(mysql)) == NULL ) 
            fprintf(stderr,"mysql_store_result error");
        return mysql_res;
    }
    return NULL;
}

void AddNewUser(char *ip)
{
	char buf[MAXLEN];
	snprintf(buf,MAXLEN,"/usr/sbin/ipset add -exist newuser %s,%c%c:%c%c:%c%c:%c%c:%c%c:%c%c timeout %d",
		ip, MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5],MAC[6],MAC[7],MAC[8],MAC[9],MAC[10],MAC[11],
		600);
	system(buf);
	fprintf(stderr,"%s:%s newuser",ip,MAC);
}

void GetMAC(char *ip) {
	FILE *fp;
	char buf[MAXLEN];
	int len;
	MAC[0] = 0;
	len = strlen(ip);
	fp = fopen("/proc/net/arp","r");
	if(!fp) return;
	while(fgets(buf,MAXLEN,fp)) {
		if(strlen(buf)<64) continue;
		if(strncmp(buf,ip,len)!=0) continue;
		if(buf[len]!=' ')  continue;
		MAC[0]=buf[41];
		MAC[1]=buf[42];
		MAC[2]=buf[44];
		MAC[3]=buf[45];
		MAC[4]=buf[47];
		MAC[5]=buf[48];
		MAC[6]=buf[50];
		MAC[7]=buf[51];
		MAC[8]=buf[53];
		MAC[9]=buf[54];
		MAC[10]=buf[56];
		MAC[11]=buf[57];
		MAC[12]=0;
	}
	fclose(fp);
#ifdef DEBUG
	fprintf(stderr,"MAC: %s\n",MAC);
#endif
}

void IPOnline( char *ip, int timespan )
{
	char buf[MAXLEN];
	snprintf(buf,MAXLEN,"/usr/sbin/ipset add -exist user %s,%c%c:%c%c:%c%c:%c%c:%c%c:%c%c timeout %d",
		ip, MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5],MAC[6],MAC[7],MAC[8],MAC[9],MAC[10],MAC[11],
		timespan );
	system(buf);
	snprintf(buf,MAXLEN,"insert into IPMACPhone values('%s', '%s','%s',now(), date_add(now(), interval %d second))",
			ip,MAC,PHONE,timespan);
	ExecSQL(buf,0);
	fprintf(stderr,"%s:%s:%s login",ip,MAC,PHONE);
}

void process_auto_login(char *ip)
{
	char buf[MAXLEN];
	MYSQL_RES *mysql_res;
	MYSQL_ROW row;
#ifdef DEBUG
	fprintf(stderr,"processing pkt from %s\n",ip);
#endif
	GetMAC(ip);
	if(MAC[0]==0) {
		fprintf(stderr,"%s 无法获取MAC地址\n",ip);
		return;
	}

	snprintf(buf,MAXLEN,"select msg from black where black='%s'",MAC);
	mysql_res = ExecSQL(buf,1);
	row = mysql_fetch_row(mysql_res);
	if( row )    {
		AddNewUser(ip);
		fprintf(stderr,"%s:%s MAC地址禁用\n",ip,MAC);
		return;
	}

	snprintf(buf,MAXLEN,"select phone,memo from VIPMAC where MAC='%s'",MAC);
	mysql_res = ExecSQL(buf,1);
	row = mysql_fetch_row(mysql_res);
	if( row ) {
		snprintf(PHONE,12,row[0]);
		snprintf(buf,MAXLEN,"insert into Log values('%s','%s','%s',now(),'VIP %s auto online 7 day')",
			ip, MAC, PHONE, row[1]);
		ExecSQL(buf,0);
#ifdef DEBUG 
		fprintf(stderr,"VIP %s:%s:%s login\n",row[1],ip,MAC);
#endif
		IPOnline(ip,7*24*3600);
		return;
	}
	snprintf(buf,MAXLEN,"select phone,timestampdiff(second,now(),end) from MACPhone where MAC='%s' and now()< end",MAC);
	mysql_res = ExecSQL(buf,1);
	row = mysql_fetch_row(mysql_res);
	if( row ) {
		snprintf(PHONE,12,row[0]);
		snprintf(buf,MAXLEN,"insert into Log values('%s','%s','%s',now(),'auto online %s sec')",
			ip, MAC, PHONE, row[1]);
		ExecSQL(buf,0);
#ifdef DEBUG 
		fprintf(stderr,"auto login %s:%s:%s login\n",row[1],ip,MAC);
#endif
		IPOnline(ip,atoi(row[1]));
		return;
	}
}

	
static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	struct nfq_data *nfa, void *data){
  	(void)nfmsg;
  	(void)data;
  	u_int32_t id = 0;
  	struct nfqnl_msg_packet_hdr *ph;
  	unsigned char *pdata = NULL;
  	int pdata_len;

  	ph = nfq_get_msg_packet_hdr(nfa);
  	if (ph){
    	id = ntohl(ph->packet_id);
  	}

  	pdata_len = nfq_get_payload(nfa, (unsigned char**)&pdata);
  	if(pdata_len == -1){
    	pdata_len = 0;
  	}

  	struct iphdr *iphdrp = (struct iphdr *)pdata;

#ifdef DEBUG
  	fprintf(stderr,"len %d iphdr %d %s ->",
    	pdata_len,
        iphdrp->ihl<<2,
        inet_ntoa(*(struct in_addr*)&iphdrp->saddr));
  	fprintf(stderr," %s",
        inet_ntoa(*(struct in_addr*)&iphdrp->daddr));
  	fprintf(stderr,"\n");
#endif
	if( iphdrp->version == 4 ) 
		process_auto_login(inet_ntoa(*(struct in_addr*)&iphdrp->saddr));

	int r= nfq_set_verdict(qh, id, NF_DROP, (u_int32_t)pdata_len, pdata);
	if ( r < 0) {
		fprintf(stderr, "nfq_set_verdict ret %d\n",r);
	}
	return 0;
}

int main(int argc, char **argv){
  	struct nfq_handle *h;
  	struct nfq_q_handle *qh;
  	struct nfnl_handle *nh;
  	int fd;
  	int rv;
  	char buf[4096];

	ConnectDB();

  	h = nfq_open();
  	if (!h) {
    	exit(1);
  	}

  	if (nfq_unbind_pf(h, AF_INET) < 0){
    	exit(1);
  	}

  	if (nfq_bind_pf(h, AF_INET) < 0) {
    	exit(1);
  	}

  	int qid = 1;
  	if(argc == 2){
    	qid = atoi(argv[1]);
  	}
  	printf("binding this socket to netfilter queue %d\n", qid);
  	qh = nfq_create_queue(h,  qid, &cb, NULL);
  	if (!qh) {
    	exit(1);
  	}

  	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
    	exit(1);
  	}

  	nh = nfq_nfnlh(h);
  	fd = nfnl_fd(nh);

  	while (1) {
		rv = recv(fd, buf, sizeof(buf), 0);
		if(rv > 0) 
    			nfq_handle_packet(h, buf, rv);
		else {
			fprintf(stderr,"recv got %d, errno=%d\n",rv,errno);
			break;
		}
  	}

  /* never reached */
  	nfq_destroy_queue(qh);

  	nfq_close(h);
	
  	exit(0);
}
