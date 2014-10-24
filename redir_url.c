
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
  
int sockfd = 0;
#define MAXLEN 2048

#define PH "202.141.166.1"

// #define DEBUG

#ifdef __LITTLE_ENDIAN
#define IPQUAD(addr) \
    ((unsigned char *)&addr)[0],                  \
    ((unsigned char *)&addr)[1],                \
    ((unsigned char *)&addr)[2],                \
    ((unsigned char *)&addr)[3]
#else
#define IPQUAD(addr)                            \
  ((unsigned char *)&addr)[3],                  \
    ((unsigned char *)&addr)[2],                \
    ((unsigned char *)&addr)[1],                \
    ((unsigned char *)&addr)[0]
#endif

// function from http://www.bloof.de/tcp_checksumming, thanks to crunsh
u_int16_t tcp_sum_calc(u_int16_t len_tcp, u_int16_t src_addr[], u_int16_t dest_addr[], u_int16_t buff[])
{
    u_int16_t prot_tcp = 6;
    u_int32_t sum = 0 ;
    int nleft = len_tcp;
    u_int16_t *w = buff;
 
    /* calculate the checksum for the tcp header and payload */
    while(nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }
 
    /* if nleft is 1 there ist still on byte left. We add a padding byte (0xFF) to build a 16bit word */
    if(nleft>0)
		sum += *w&ntohs(0xFF00);   /* Thanks to Dalton */
 
    /* add the pseudo header */
    sum += src_addr[0];
    sum += src_addr[1];
    sum += dest_addr[0];
    sum += dest_addr[1];
    sum += htons(len_tcp);
    sum += htons(prot_tcp);
 
    // keep only the last 16 bits of the 32 bit calculated sum and add the carries
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
 
    // Take the one's complement of sum
    sum = ~sum;
 
    return ((u_int16_t) sum);
}

static void set_tcp_checksum(struct iphdr* ip){
  struct tcphdr *tcph = (struct tcphdr*)((u_int8_t*)ip + (ip->ihl<<2));
	tcph->check = 0; /* Checksum field has to be set to 0 before checksumming */
	tcph->check = (u_int16_t) tcp_sum_calc((u_int16_t) (ntohs(ip->tot_len) - ip->ihl *4), 
		(u_int16_t*) &ip->saddr, (u_int16_t*) &ip->daddr, (u_int16_t *) tcph); 

}

void swap_bytes( unsigned char *a, unsigned char *b, int len){
	unsigned char t;
	int i;
	if(len<=0) return;
	for(i=0;i<len;i++) {
		t=*(a+i);
		*(a+i) = * (b+i);
		*(b+i)=t;
	}
}

void process_packet( unsigned char *pkt, int pkt_len) {
static int long pkts;
	if( pkt_len > MAXLEN ) pkt_len = MAXLEN;
	struct iphdr *ip = (struct iphdr *) pkt;
	struct tcphdr *tcph = (struct tcphdr*) (pkt + ip->ihl*4);	
	pkts++;
#ifdef DEBUG
	fprintf(stderr,"processing pkt %ld\n",pkts);
#endif
	
	if( tcph->syn  && (!tcph->ack) ) {    // SYN packet, send SYN+ACK
		struct sockaddr_in to;
#ifdef DEBUG
		fprintf(stderr,"SYN packet\n");
#endif
		swap_bytes((unsigned char *)&ip->saddr, (unsigned char *)&ip->daddr, 4);
		swap_bytes((unsigned char *)&tcph->source,(unsigned char *)&tcph->dest,2);
		tcph->ack = 1;
		tcph->ack_seq =  htonl( ntohl(tcph->seq) +1 );
		tcph->seq = htonl(1);
		tcph->doff = 20/4;
		pkt_len = ip->ihl *4 +  tcph->doff *4;
		ip->tot_len = htons ( pkt_len);
		ip->check = 0;
		set_tcp_checksum(ip);		
		to.sin_addr.s_addr = ip->daddr;
    		to.sin_family = AF_INET;
    		to.sin_port = tcph->dest;
		int r = sendto(sockfd,pkt, pkt_len,0,(const struct sockaddr *)&to, sizeof(to));
		if(r<0) 
			perror("sendto");
#ifdef DEBUG
		fprintf(stderr,"sendto return %d\n",r);
#endif
	} else if( tcph->fin ) {    // FIN packet, send ACK
                struct sockaddr_in to;
#ifdef DEBUG
                fprintf(stderr,"FIN packet\n");
#endif
                swap_bytes((unsigned char *)&ip->saddr, (unsigned char *)&ip->daddr, 4);
                swap_bytes((unsigned char *)&tcph->source,(unsigned char *)&tcph->dest,2);
                swap_bytes((unsigned char *)&tcph->seq,(unsigned char *)&tcph->ack_seq,4);
                tcph->ack = 1;
                tcph->ack_seq =  htonl( ntohl(tcph->ack_seq) +1 );
                tcph->doff = 20/4;
                pkt_len = ip->ihl *4 + tcph->doff *4;
                ip->tot_len = htons (pkt_len);
                ip->check = 0;
                set_tcp_checksum(ip);
                to.sin_addr.s_addr = ip->daddr;
                to.sin_family = AF_INET;
                to.sin_port = tcph->dest;
                int r = sendto(sockfd,pkt, pkt_len,0,(const struct sockaddr *)&to, sizeof(to));
                if(r<0) 
                        perror("sendto");
#ifdef DEBUG
                fprintf(stderr,"sendto return %d\n",r);
#endif
	} else if( tcph->ack && (!tcph->syn) ) {    // ACK packet, send DATA
		int tcp_payload_len = pkt_len - ip->ihl*4 - tcph->doff*4;
		int ntcp_payload_len, npkt_len;
		unsigned char *tcp_payload;
		unsigned char *ntcp_payload;
		struct sockaddr_in to;
		unsigned char buf[MAXLEN];

#ifdef DEBUG
		fprintf(stderr,"ACK pkt len=%d\n",pkt_len);
#endif
#if 0
		fprintf(stderr,"payload len=%d\n",tcp_payload_len);
#endif
		if ( tcp_payload_len <= 5 ) {
#ifdef DEBUG
			fprintf(stderr,"tcp payload len=%d too small, ignore\n",tcp_payload_len);
			fprintf(stderr,"packet processed\n\n");
#endif
			return;
		}
		tcp_payload = pkt + ip->ihl*4 + tcph->doff*4;
		if ( (*tcp_payload !='G') || 
			(*(tcp_payload+1) !='E') || 
			(*(tcp_payload+2) !='T') || 
			(*(tcp_payload+3) !=' ')  ) {
#ifdef DEBUG
			fprintf(stderr,"not HTTP GET, ignore\n");
			fprintf(stderr,"packet processed\n\n");
#endif
			return;
		}
		int i;
#ifdef DEBUG
		fprintf(stderr,"payload is: ");
		for(i=0;i<tcp_payload_len;i++) {
	//			if(*(tcp_payload+i)=='\r') break;
			fprintf(stderr,"%c",*(tcp_payload+i));
		}
		fprintf(stderr,"\n");
#endif

		unsigned char *url = tcp_payload+4;
		for(i=4; i<tcp_payload_len-5; i++)
			if(*(tcp_payload+i)==' ') break; 
		*(tcp_payload+i) = 0;
		i++;
		unsigned char *host;
		unsigned char hostip[100];
		host = memmem(tcp_payload+i, tcp_payload_len-i-2, "Host: ",6);
		if ( host ) {
			host = host + 6;
			for( i = host - tcp_payload; i< tcp_payload_len-2; i++)
				if( (*(tcp_payload+i)=='\n') ||  (*(tcp_payload+i)=='\r')) break;
			*(tcp_payload+i) = 0;
		} else {
			snprintf((char*)hostip,80,"%u.%u.%u.%u", IPQUAD(ip->daddr));
			host = hostip;
		}
		unsigned char *agent;
		agent = memmem(tcp_payload, tcp_payload_len-2, "User-Agent: ",12);
		if ( agent ) {
			agent = agent + 12;
			for( i = agent - tcp_payload; i< tcp_payload_len-2; i++)
				if( (*(tcp_payload+i)=='\n') ||  (*(tcp_payload+i)=='\r')) break;
			*(tcp_payload+i) = 0;
		} else 
			agent = (unsigned char *) ""; 
#if	0
		static int print_url = 0;
		if(print_url < 100) {
			print_url ++;
			fprintf(stderr,"url is http://%s%s\n",host,url);	
		}
#endif
		if ( strncmp((char*)url,"/library/test/success.html",26) == 0 ) return;
		if ( strncmp((char*)url,"/wangwang/get_task.php",22) == 0 ) return;
		if ( strncmp((char*)url,"/list=sys_hqEtagMode",20) == 0 ) return;
		if ( strncmp((char*)url,"/editor/guy.gif",15) == 0 ) return;
		if ( strncmp((char*)url,"/webservice.php",15) == 0 ) return;
		if ( strncmp((char*)url,"/sync/get?uid=",14) == 0 ) return;
		if ( strncmp((char*)url,"/announce.php",13) == 0 ) return;
		if ( strncmp((char*)url,"/etag.php?rn=",13) == 0 ) return;
		if ( strncmp((char*)url,"/generate_204",13) == 0 ) return;
		if ( strncmp((char*)url,"/player.htm?",13) == 0 ) return;
		if ( strncmp((char*)url,"/getts.json",11) == 0 ) return;
		if ( strncmp((char*)url,"/res/static",11) == 0 ) return;
		if ( strncmp((char*)url,"/din.aspx?",10) == 0 ) return;
		if ( strncmp((char*)url,"/comet_get",10) == 0 ) return;
		if ( strncmp((char*)url,"/announce?",10) == 0 ) return;
		if ( strncmp((char*)url,"/v1/route",9) == 0 ) return;
		if ( strncmp((char*)url,"/profile/",9) == 0 ) return;
		if ( strncmp((char*)url,"/1.html?",8) == 0 ) return;
		if ( strncmp((char*)url,"/?qt=rg",7) == 0 ) return;
		if ( strncmp((char*)url,"/api/",5) == 0 ) return;
		if ( strncmp((char*)url,"/p2p?",5) == 0 ) return;
		if ( strcmp((char*)url,"/abc") == 0 ) return;

		if ( strcmp((char*)host,PH) ==0 ) return;
		if ( strcmp((char*)host,"mirrors.hust.edu.cn") ==0 ) return;
		if ( strcmp((char*)host,"sdup.360.cn") ==0 ) return;
		if ( strcmp((char*)host,"update.360safe.com") ==0 ) return;
		if ( strcmp((char*)host,"jipiao.jd.com") ==0 ) return;
		if ( strcmp((char*)host,"jipiao.kuxun.cn") ==0 ) return;
		if ( strcmp((char*)host,"flight.elong.com") ==0 ) return;
		if ( strcmp((char*)host,"static.tieba.baidu.com") ==0 ) return;
		if ( strcmp((char*)host,"message.tieba.baidu.com") ==0 ) return;

		if ( (strncmp((char*)agent,"Wget",4)==0) ||
		     (strncmp((char*)agent,"Debian APT-HTTP",15)==0) 
		) {
			return;
		}
#if	1
		static int print_url = 0;
		print_url ++;
		if(print_url % 500 == 0) {
			time_t tm;
			print_url = 0;
    			time(&tm);
			fprintf(stderr,"%s",ctime(&tm));
			fprintf(stderr,"url is http://%s%s\n",host,url);	
			fprintf(stderr,"agent %s\n",agent);
		}
#endif
		// copy new packet
		memcpy((void*)buf,(void*)pkt,(size_t )(ip->ihl *4 + tcph->doff*4));
		struct iphdr *nip = (struct iphdr *) buf;
		struct tcphdr *ntcph = (struct tcphdr*) (buf + ip->ihl *4);	

		ntcp_payload = buf + nip->ihl *4 + ntcph->doff*4;

#if 0
		char * http_redirect_header =
			"HTTP/1.1 302 Moved Temporarily\r\n"
			"Location: http://" PH "/\r\n"
			"Content-Type: text/html; charset=iso-8859-1\r\n"
			"\r\n%s"  ;
		char * http_redirect_body =    
    			"<html><head>\n"  
    			"<title>302 Moved Temporarily</title>\n"  
    			"</head><body>\n"  
    			"<h1>Moved Teporarily</h1>\n"  
    			"<p>The document has moved <a href=\"http://" PH "/cgi-bin/ip\">here</a>.</p>\n"  
    			"</body></html>\n";  
#endif
		        char * http_redirect_content1 =
                        "HTTP/1.1 200 OK\r\n"
			"Server: redir_url by james@ustc.edu.cn\r\n"
                        "Content-Type: text/html; charset=iso-8859-1\r\n"
			"Cache-Control: no-cache, must-revalidate\r\n"
			"Pragma: no-cache\r\n"
			"Connection: close\r\n"
                        "\r\n"
                        "<html><head>\n"  
                        "<title>redirecting to http://" PH "</title>\n"   
                        "</head><body>\n"  
                        "<h1>redirecting to http://" PH "</h1>\n"  
			"<script language=\"javascript\">"
    			"window.location.href = \"http://" PH "/cgi-bin/ip?url=http://%s%s&dip=%u.%u.%u.%u\";</script>"
                        "<p>redirecting to <a href=\"http://" PH "/\">http://" PH "/</a>.</p>\n"
                        "</body></html>\n";     

		        char * http_redirect_content2 =
                        "HTTP/1.1 200 OK\r\n"
			"Server: redir_url by james@ustc.edu.cn\r\n"
                        "Content-Type: text/html; charset=iso-8859-1\r\n"
			"Cache-Control: no-cache, must-revalidate\r\n"
			"Pragma: no-cache\r\n"
			"Connection: close\r\n"
                        "\r\n"
                        "<html><head>\n"  
                        "<title>redirecting to http://" PH "</title>\n"   
                        "</head><body>\n"  
                        "<h1>redirecting to http://" PH "</h1>\n"  
			"<script language=\"javascript\">"
    			"window.location.href = \"http://" PH "/cgi-bin/ip\";</script>"
                        "<p>redirecting to <a href=\"http://" PH "/\">http://" PH "/</a>.</p>\n"
                        "</body></html>\n";   

		if(strlen((char*)host)+strlen((char*)url)<200) 
		ntcp_payload_len = snprintf((char *)ntcp_payload, 1200, 
			http_redirect_content1,host,url,IPQUAD(ip->daddr));
		else
		ntcp_payload_len = snprintf((char *)ntcp_payload, 1200, 
			http_redirect_content2);
#ifdef DEBUG
		fprintf(stderr,"new payload len=%d\n",ntcp_payload_len);
#endif
#if 0
		fprintf(stderr,"new payload is: ");
		for(i=0;i<ntcp_payload_len;i++)
			fprintf(stderr,"%c",*(ntcp_payload+i));
#endif
		npkt_len = ntcp_payload_len + nip->ihl*4 + ntcph->doff*4;
		nip->tot_len = htons (npkt_len);
#ifdef DEBUG
		fprintf(stderr,"new pkt len=%d\n",npkt_len);
#endif
                swap_bytes((unsigned char *)&nip->saddr, (unsigned char *)&nip->daddr, 4);
                swap_bytes((unsigned char *)&ntcph->source,(unsigned char *)&ntcph->dest,2);
                ntcph->ack = 1;
                ntcph->psh = 1;
                ntcph->fin = 1;
                ntcph->seq =  htonl( ntohl(tcph->ack_seq) );
                ntcph->ack_seq =  htonl( ntohl(tcph->seq) + tcp_payload_len );
                nip->check = 0;
                set_tcp_checksum(nip);
                to.sin_addr.s_addr = nip->daddr;
                to.sin_family = AF_INET;
                to.sin_port = ntcph->dest;
                int r = sendto(sockfd,buf,npkt_len,0,(const struct sockaddr *)&to, sizeof(to));
                if(r<0) 
                        perror("sendto");
#ifdef DEBUG
                fprintf(stderr,"sendto return %d\n",r);
#endif
	}
#ifdef DEBUG
	fprintf(stderr,"packet processed\n\n");
#endif
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
  	fprintf(stderr,"len %d iphdr %d %u.%u.%u.%u ->",
    	pdata_len,
        iphdrp->ihl<<2,
        IPQUAD(iphdrp->saddr));
  	fprintf(stderr," %u.%u.%u.%u",
        IPQUAD(iphdrp->daddr));
  	fprintf(stderr,"\n");
#endif
	if(   ( iphdrp->version == 4 ) 
		&& ( (ntohs(iphdrp->frag_off) & 0x1fff) == 0 )
		&& ( iphdrp->protocol == IPPROTO_TCP )
		&& ( ntohs(iphdrp->tot_len) <= pdata_len ) )
		process_packet(pdata, pdata_len);

	int r= nfq_set_verdict(qh, id, NF_DROP, (u_int32_t)pdata_len, pdata);
	if ( r < 0) {
		fprintf(stderr, "nfq_set_verdict ret %d\n",r);
		sleep (2);
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

  	printf("open rawsocket\n");
  	if(0>(sockfd=socket(AF_INET,SOCK_RAW,IPPROTO_RAW))){  // IPPROTO_RAW means only send
		perror("Create Error");
		exit(1);
	}

	const int on = 1;
	if(0>setsockopt(sockfd,IPPROTO_IP,IP_HDRINCL,&on,sizeof(on))){
		perror("IP_HDRINCL failed");
		exit(1);
	}

  	int qid = 0;
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
