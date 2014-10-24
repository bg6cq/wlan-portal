#include "cgilib.c"
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

#define	RATEINTERVAL	2
#define RATECOUNT		20

#define MAXPERMAC  5
#define MAXPERPHONE 2

//#define DEBUG 1


char PHONE [MAXLEN];
char MAC [MAXLEN];
char DeferMSG [MAXLEN];

MYSQL *mysql = NULL;

int HtmlHeadOut = 0;

void DisplayStage(char s, char *msg);

/* ����mysql���ݿ� */
MYSQL * ConnectDB(void)
{
	if( (mysql=mysql_init(NULL))==NULL ) 
       	DisplayStage('0',"�ڲ�����mysql_init error");
	if( mysql_real_connect(mysql, AUTHDBHOST, AUTHDBUSER, AUTHDBPASSWD,
		AUTHDB, AUTHDBPORT, AUTHDBSOCKPATH, 0)== NULL)
       	DisplayStage('0',"�ڲ�����mysql_connect error");
    return mysql;
}

/* ִ��sql��� */
MYSQL_RES * ExecSQL(char *sql, int haveresult)
{
    MYSQL_RES *mysql_res;
    if( mysql_query(mysql,sql) )
        DisplayStage('0',"�ڲ�����mysql_querying error");
    if( haveresult ) {
        if( (mysql_res = mysql_store_result(mysql))==NULL ) 
            DisplayStage('0',"�ڲ�����mysql_store_result error");
        return mysql_res;
    }
    return NULL;
}

void title_body(void) 
{
	printf("<htm><head><title>WLAN-Portal</title>\n");
	printf("<META http-equiv=Content-Type content=\"text/html; charset=gb2312\">\n");
	printf("<meta http-equiv=\"pragma\" content=\"no-cache\" />\n");
	printf("<meta http-equiv=\"Cache-Control\" CONTENT=\"no-cache\">\n");
	printf("<style type=\"text/css\"> <!-- a:link { color: #0000ff; text-decoration: underline; } a:visited { color: #0000ff; text-decoration: underline; } --> </style>\n");
	printf("</head><body bgcolor=\"#dddddd\" text=\"#000000\">\n");
}

void HtmlHead(void) {
	if( HtmlHeadOut==0 ) 
		printf("Content-type: text/html\r\n\r\n");
	HtmlHeadOut = 1;
}

void HtmlHeadCookie(char *cookie) {
	if( HtmlHeadOut==0 ) {
		printf("Content-type: text/html\r\n");
		printf("Set-Cookie: %s\r\n\r\n",cookie);
		HtmlHeadOut = 1;
	} else 
		printf("bug: HTML Head has sent\n");
}

char * FilterLine ( char * line) {
	static char buf[MAXLEN];
	char * url;
	
   	int i=0;
   	char * s, *p, *ps;

	p = GetValue("url");
	if( (p==0) || (*p==0) ) 
			url=NULL;
	else url=p;

   	p=line;
   	while(*p) {
		s = "USERIP";
		if((ps=strstr(p,s))) {
			strncpy(buf+i,p,ps-p);  i+=ps-p;
			strcpy(buf+i,remote_addr()); i+=strlen(remote_addr());
			p=ps+strlen(s); continue;
		}
		s = "MACADDR";
		if((ps=strstr(p,s))) {
			strncpy(buf+i,p,ps-p);  i+=ps-p;
			strcpy(buf+i,MAC); i+=strlen(MAC);
			p=ps+strlen(s); continue;
		}
		s = "MSG";
		if((ps=strstr(p,s))) {
			strncpy(buf+i,p,ps-p);  i+=ps-p;
			strcpy(buf+i,DeferMSG); i+=strlen(DeferMSG);
			p=ps+strlen(s); continue;
		}
		s = "PHONE";
		if((ps=strstr(p,s))) {
			strncpy(buf+i,p,ps-p);  i+=ps-p;
			strcpy(buf+i,PHONE); i+=strlen(PHONE);
			p=ps+strlen(s); continue;
		}
		
		if(url) {
			s = "URL";
			if((ps=strstr(p,s))) {
				strncpy(buf+i,p,ps-p);  i+=ps-p;
				strcpy(buf+i,url); i+=strlen(url);
				p=ps+strlen(s); continue;
			}
		}

		strcpy(buf+i,p);
		break;
	}
    return buf;
}

void PrintFile(char *fname) {
	char buf[MAXLEN];
	FILE *fp;
	fp = fopen(fname,"r");
	if(fp) {
		while(fgets(buf,MAXLEN,fp))
			printf("%s",FilterLine(buf));
	}
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
}

void DisplayStage(char s, char *msg)
{
	char *p;
	char buf[MAXLEN];
	if(msg!=NULL && *msg!=0) 
		snprintf(DeferMSG,MAXLEN,"��Ϣ��%s",msg);
	p = user_agent();
	if ( (s<'0') ||  (s>'2') ) 
		s='0';

	if( HtmlHeadOut==0 ) {
		HtmlHead();
		title_body(); 
	}
	if( p && (strstr(p,"Mobile")) ) {
#ifdef DEBUG
		snprintf(buf,MAXLEN,"/var/www/html/mstage%c.1.html",s);
#else
		snprintf(buf,MAXLEN,"/var/www/html/mstage%c.html",s);
#endif
	} else {
#ifdef DEBUG
		snprintf(buf,MAXLEN,"/var/www/html/stage%c.1.html",s);
#else
		snprintf(buf,MAXLEN,"/var/www/html/stage%c.html",s);
#endif
	}
	PrintFile(buf);
	if ( mysql ) mysql_close(mysql);
	exit(0);
}


void IPOnline( void )
{
	char buf[MAXLEN];
	char *url;
	setuid(0);
	snprintf(buf,MAXLEN,"/usr/sbin/ipset add -exist user %s,%c%c:%c%c:%c%c:%c%c:%c%c:%c%c",
		remote_addr(), MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5],MAC[6],MAC[7],MAC[8],MAC[9],MAC[10],MAC[11]);
	system(buf);
	snprintf(buf,MAXLEN,"insert into IPMACPhone values('%s', '%s','%s',now())",
			remote_addr(),MAC,PHONE);
	ExecSQL(buf,0);
	url=GetValue("url");
	if(url && (*url) && (strcmp(url,"URL")!=0)) {
		if( HtmlHeadOut==0 ) {
			HtmlHead();
			title_body(); 
		}
		printf("<script language=\"javascript\" type=\"text/javascript\">"
                        "window.location.href=\"%s\";</script>\n", url);
		printf("���������<a href=%s>%s</a>",url,url);
		mysql_close(mysql);
		exit(0);
	}
	DisplayStage('2',"��ӭʹ������");

}

void CheckPhone(char*phone)
{
	char *p;
	if(strlen(phone)!=11) 
		DisplayStage('0',"�绰���������11λ����");
	for(p=phone;*p;p++) {
		if(*p>='0' && *p<='9') continue;
		DisplayStage('0',"�绰�������������");
	}
	if(*phone!='1') 
		DisplayStage('0',"�绰�������������1��ͷ");
}

void Stage0(void) 
{
	char buf[MAXLEN];
	MYSQL_RES *mysql_res;
	MYSQL_ROW row;
	snprintf(buf,MAXLEN,"select phone from MACPhone where MAC='%s' and now()< end",MAC);
	mysql_res = ExecSQL(buf,1);
	row = mysql_fetch_row(mysql_res);
	if( row )    {
		snprintf(PHONE,12,row[0]);
		snprintf(buf,MAXLEN,"insert into Log values('%s','%s',now(),'%s auto online')",
			remote_addr(), MAC, PHONE);
		ExecSQL(buf,0);
		IPOnline();
	}
	DisplayStage('0',NULL);	
	exit(0);
}

void Stage1() // sendsms, dispay input page
{
	char *phone,*p;
   	char buf[MAXLEN];
	char pass[MAXLEN];
	MYSQL_RES *mysql_res;
	MYSQL_ROW row;
	phone = GetValue("phone");
	if( phone==NULL || phone[0]==0 ) 
		DisplayStage('0',"����ĵ绰����Ϊ��");
	CheckPhone(phone);
	strncpy(PHONE,phone,12);	
	p = GetValue("havepass");
	if(p) 
		DisplayStage('1',"����������");

	// �����豸�����Ƿ��͹�����, ÿ����� MAXPERMAC
	snprintf(buf,MAXLEN,"select count from MACcount where MAC='%s' and sendday=curdate()",MAC);
	mysql_res = ExecSQL(buf,1);
	row = mysql_fetch_row(mysql_res);
	if( row )    {
		if( atoi(row[0]) >= MAXPERMAC ) {
			sprintf(buf,"ÿ̨�豸ÿ������%d�ֻ���¼�������Ѿ�ʹ��%s�Σ��뻻̨�豸����",MAXPERMAC,row[0]);
			DisplayStage('0',buf);
		} 
		snprintf(buf,MAXLEN,"update MACcount set count=count+1 where MAC='%s' and sendday=curdate()",MAC);
		ExecSQL(buf,0);
	} else {
		snprintf(buf,MAXLEN,"replace into MACcount values ('%s', now(), 1)",MAC);
		ExecSQL(buf,0);
	}
	
	// ����ֻ������Ƿ��͹�����, ÿ����� MAXPERPHONE
	snprintf(buf,MAXLEN,"select count from Phonecount where phone='%s' and sendday=curdate()",PHONE);
	mysql_res = ExecSQL(buf,1);
	row = mysql_fetch_row(mysql_res);
	if( row )    {
		if( atoi(row[0]) >= MAXPERPHONE ) {
			sprintf(buf,"ÿ���ֻ�ÿ������%d���ţ������Ѿ�ʹ��%s�Σ��뻻�ֻ�����",MAXPERPHONE,row[0]);
			DisplayStage('0',buf);
		} 
		snprintf(buf,MAXLEN,"update Phonecount set count=count+1 where phone='%s' and sendday=curdate()",PHONE);
		ExecSQL(buf,0);
	} else {
		snprintf(buf,MAXLEN,"replace into Phonecount values ('%s', now(), 1)",PHONE);
		ExecSQL(buf,0);
	}
	srand(time(NULL));
	int i;
	for(i=0;i<6;i++) {
		int r;
		r=rand();
		pass[i]='0' + r%10;
	}
	pass[6]=0;
	snprintf(buf,MAXLEN,"replace into PhonePass values ('%s', '%s')",PHONE,pass);
	ExecSQL(buf,0);
	snprintf(buf,MAXLEN,"insert into Log values('%s','%s',now(),'send pass to %s')",
		remote_addr(), MAC, PHONE);
	ExecSQL(buf,0);
	snprintf(buf,MAXLEN,"php /usr/src/sendsms/sendsms.php %s \"�����й��ƴ�WLAN��������%s\" >/dev/null 2>/dev/null",PHONE,pass);
	system(buf);
	DisplayStage('1',"�������ֻ����յ�������");
}

void Stage2() // setonline
{
	char *phone,*password;
   	char buf[MAXLEN];
	MYSQL_RES *mysql_res;
	MYSQL_ROW row;
	phone = GetValue("phone");
	if( phone==NULL || phone[0]==0 ) 
		DisplayStage('0',"����ĵ绰����Ϊ��");
	CheckPhone(phone);
	strncpy(PHONE,phone,12);	
	password = GetValue("password");
	if((password==NULL) || strlen(password)!=6) 
		DisplayStage('1',"����������");
	
	snprintf(buf,MAXLEN,"select pass from PhonePass where phone='%s'",phone);
	mysql_res = ExecSQL(buf,1);
	row = mysql_fetch_row(mysql_res);
	if( row==NULL )   
       		DisplayStage('0',"�绰����δע��,����������");
	if( strcmp(row[0],password)!=0 ) {
       		DisplayStage('1',"�������,����������");
	}
	snprintf(buf,MAXLEN,"replace into MACPhone values('%s','%s',now(), '2035-1-1 00:00:00')",
			MAC,PHONE);
	ExecSQL(buf,0);
	snprintf(buf,MAXLEN,"insert into Log values('%s','%s',now(),'%s online')",
		remote_addr(), MAC, PHONE);
	ExecSQL(buf,0);

	IPOnline();	
}


int CGImain(void) {
	char *s;
	ConnectDB();
	s = GetValue("s");
	GetMAC(remote_addr());
	if ( (s== NULL) || *s=='0') 
		Stage0();
	else if( *s=='1') 
		Stage1();
	else if(*s=='2') {
		 Stage2();
	} else Stage0();
	return 0;
}
