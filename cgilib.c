#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXERR 6

#define MEMERROR 	-1  /* No more memory */
#define ERRORNOMETHOD 	-2  /* No request method */
#define NOCONTENTLEN	-3  /* No content_lengh when method is PUT or POST */
#define ERRORTYPE	-4  /* Unknow Content type */
#define TOOLONGKEY	-5  /* too lon cookie key */

char * ErrMsg[MAXERR] =  {
"",
"No more memory!",
"No requst method!",
"No content_lengh when method is PUT or POST!",
"Unknow Content type!",
"Too Long Cookie Key!" };

void ErrorExit(int msgno)
{
	printf("Content-type: text/html\n\n");
	if(msgno>=0 || msgno <=-MAXERR) {
		printf("Unknow Error Message!\n");
	}
	else printf("Error Message: %s\n",ErrMsg[-msgno]);
	fflush(stdout);
	exit(0);
}

#define MAXLEN 1024

typedef struct _string {
	int memlen;
	int charlen;
	char *s;
} STRING;

STRING * StringNew(void)
{ 	STRING * t;
	if( (t=malloc(sizeof(STRING)))==NULL) ErrorExit(MEMERROR);
	t->memlen=0;
	t->charlen=0;
	t->s = NULL;
	return t;
}

int StringLen(STRING *s) { return (s==NULL ? 0: s->charlen); }

void StringDestroy(STRING *s) {	free(s->s); free(s); }

STRING *StringCat(STRING *dest, STRING *src)
{
	int dlen=dest->charlen;
	int slen=src->charlen;
	if(dest->memlen<dlen+slen+1) {
		dest->memlen=dlen+slen+ ((dlen+slen)>>3) +5;
		if((dest->s=realloc(dest->s,dest->memlen))==NULL) ErrorExit(MEMERROR);
	}	
	memcpy(dest->s+dest->charlen,src->s,slen);
	dest->charlen+=slen;
	dest->s[dest->charlen]=0;
	return dest;
}

char * GetPChar(STRING *s) { if(s)return s->s;else return NULL; }

STRING *StringCatPChar(STRING *dest, char *s)
{
	int dlen=dest->charlen;
	int slen=strlen(s);
	if(dest->memlen<dlen+slen+1) {
		dest->memlen=dlen+slen+ ((dlen+slen)>>3)+5 ;
		if((dest->s=realloc(dest->s,dest->memlen))==NULL) ErrorExit(MEMERROR);
	}	
	strcpy(dest->s+dest->charlen,s);
	dest->charlen+=slen;
	return dest;
}

STRING *StringAddChar(STRING *dest, char c)
{
	int dlen=dest->charlen;
	if(dest->memlen<dlen+2) {
		dest->memlen=dlen+1+(dlen>>3)+5;
		if((dest->s=realloc(dest->s,dest->memlen))==NULL) ErrorExit(MEMERROR);
	}	
	dest->s[dest->charlen]=c;
	dest->charlen++;
	dest->s[dest->charlen]=0;
	return dest;
}

STRING * PCharToString(char *p)
{
	STRING * t;
	t= StringNew();
	return StringCatPChar(t,p);	
}


typedef struct  _param {
	STRING *name;
	STRING *value;
	struct _param *next;
} PARAM;

PARAM * HeadOfParam=NULL;
PARAM * LastParam=NULL;

PARAM * NewParam(STRING *n, STRING *v)
{
	PARAM *t;
	t = malloc(sizeof(PARAM));
	if(t==NULL) ErrorExit(MEMERROR);
	t->name = n;
	t->value=v;
	t->next = NULL;
	return t;
}

char * GetParamName(PARAM *p )
{
	if(p==NULL) return NULL;
	else return(GetPChar(p->name));
}

char *GetParamValue(PARAM *p)
{	if(p==NULL) return NULL;
	else return(GetPChar(p->value));
}
	
PARAM * GetFirstParam(void)
{	LastParam = HeadOfParam;
	return HeadOfParam;
}

PARAM * GetNextParam(void)
{	if(LastParam==NULL) LastParam = HeadOfParam;
	else LastParam = LastParam->next;
	return LastParam;
}

char * GetValue(char * name)
{	PARAM *t;
	t = HeadOfParam;
	while(t) {
		if(strcasecmp(GetPChar(t->name),name)==0) {
			LastParam=t;
			return GetPChar(t->value);
		}
		t = t->next;
	}
	return NULL;
}

char * GetNextValue(char *name)
{	PARAM *t;
	t = LastParam->next;
	while(t) {
		if(strcasecmp(GetPChar(t->name),name)==0) {
			LastParam=t;
			return GetPChar(t->value);
		}
		t = t->next;
	}
	return NULL;
}

void AddToTail(PARAM *p)
{	PARAM *t;
	if(HeadOfParam==NULL) {
		HeadOfParam=p; return; 
	}
	t = HeadOfParam;
	while(t->next!=NULL) t = t->next;
	t->next = p;
}

typedef struct _upldfile {
	STRING *name;
	STRING *filename;
	long int len;
	char * data;
	struct _upldfile *next;
} UPLDFILE;

UPLDFILE *HeadOfUPLDFile = NULL;
UPLDFILE *LastUPLDFile = NULL;


UPLDFILE * NewUPLDFile(STRING *name, STRING *filename, long int len, char *data)
{
	UPLDFILE *t;
	t = malloc(sizeof(UPLDFILE));
	if(t==NULL) ErrorExit(MEMERROR);
	t->name = name;
	t->filename=filename;
	t->len = len;
	t->data = malloc(len+1);
	if(t->data==NULL) ErrorExit(MEMERROR);
	t->data[len]=0;
	memcpy(t->data,data,len);
	t->next = NULL;
	return t;
}

char * GetUPLDParamName(UPLDFILE *c )
{
	if(c==NULL) return NULL;
	else return(GetPChar(c->name));
}

char *GetUPLDFileName(UPLDFILE *c)
{	if(c==NULL) return NULL;
	else return(GetPChar(c->filename));
}

long int GetUPLDFileLen(UPLDFILE *c)
{	if(c==NULL)return 0;
	else return c->len;
}	

char * GetUPLDFileData(UPLDFILE *c)
{	if(c==NULL)return NULL;
	else return c->data;
}

UPLDFILE * GetFirstUPLDFile(void)
{	LastUPLDFile= HeadOfUPLDFile;
	return HeadOfUPLDFile;
}

UPLDFILE * GetNextUPLDFile(void)
{	if(LastUPLDFile==NULL) return NULL;
	LastUPLDFile= LastUPLDFile->next;
	return LastUPLDFile;
}

UPLDFILE * GetUPLDFileByName(char * name)
{	UPLDFILE *c;
	c = HeadOfUPLDFile;
	while(c) {
		if(strcasecmp(GetPChar(c->name),name)==0) {
			LastUPLDFile = c;
			return c;
		}
		c = c->next;
	}
	return NULL;
}

UPLDFILE *GetNextUPLDFileByName(char *name)
{	UPLDFILE*c;
	c = LastUPLDFile;
	while(c) {
		if(strcasecmp(GetPChar(c->name),name)==0) {
			LastUPLDFile=c;
			return c;
		}
		c = c->next;
	}
	return NULL;
}

void AddToUPLDFileTail(UPLDFILE *p)
{	UPLDFILE *c;
	if(HeadOfUPLDFile==NULL) {
		HeadOfUPLDFile=p; return; 
	}
	c = HeadOfUPLDFile;
	while(c->next!=NULL) c = c->next;
	c->next = p;
}



unsigned char x2c(char x) {
 	if ( isdigit(x) )
 		return x - '0' ;
 	else if (islower(x))
 		return x - 'a' + 10 ;
 	else
 		return x - 'A' + 10 ;
}

unsigned char c2x(unsigned char c) {
 	return c > 9 ? 'A'+c-10 : c+'0' ;
}

char * escape_char(char c) {
static char ret[10] ;
  ret[0] = '%' ;
  ret[1]= c2x((c&0xf0)>>4) ;
  ret[2]= c2x(c&0xf) ;
  ret[3]=0;
  return ret ;
}

STRING * www_escape(char * s) {
	STRING *t;
	t = StringNew();
	while(*s) {
		if(isdigit(*s) || isalpha(*s) || (*s==':') || (*s=='/') || (*s=='.') ) 
			StringAddChar(t,*s);
		else StringCatPChar(t,escape_char(*s));
		s++;
	}
	return t;
}

int ishex(char c)
{
	if(  (c>='0'&& c<='9') ||
	     (c>='a'&& c<='z') ||
	     (c>='A'&& c<='Z') ) return 1;
	else return 0;

}
STRING * www_unescape(char * s) {
	STRING *t;
	t = StringNew();
	while(*s) {
		if(*s=='+') StringAddChar(t,' ');
		else if(*s=='%' && ishex(*(s+1)) && ishex(*(s+2))) {
			StringAddChar(t,(x2c(*(s+1))<<4)+x2c(*(s+2)));
			s+=2;
		}else StringAddChar(t,*s);
		s++;
	}
	return t;
}

STRING * html_escape(char* s) {
	STRING *t;
	t = StringNew();
  	while (*s) {
    		switch (*s) {
      			case '&' : StringCatPChar(t,"&amp;"); break ;
      			case '<' : StringCatPChar(t,"&lt;"); break ;
      			case '>' : StringCatPChar(t,"&gt;"); break ;
      			default : StringAddChar(t,*s); break ;
    		}
    		s++ ;
  	}
	return t;
}


char * auth_type() { return (char*)getenv("AUTH_TYPE") ; }
char * content_length() { return (char*)getenv("CONTENT_LENGTH") ; }
char * content_type() { return (char*)getenv("CONTENT_TYPE") ; }
char * gateway_interface() { return (char*)getenv("GATEWAY_INTERFACE") ; }
char * path_info() { return (char*)getenv("PATH_INFO") ; }
char * path_translated() { return (char*)getenv("PATH_TRANSLATED") ; }
char * query_string() { return (char*)getenv("QUERY_STRING") ; }
char * remote_addr() { return (char*)getenv("REMOTE_ADDR") ; }
char * remote_host() { return (char*)getenv("REMOTE_HOST") ; }
char * remote_ident() { return (char*)getenv("REMOTE_IDENT") ; }
/* remote_port() is an Apache-specific extension */
char * remote_port() { return (char*)getenv("REMOTE_PORT") ; }
char * remote_user() { return (char*)getenv("REMOTE_USER") ; }
char * request_method() { return (char*)getenv("REQUEST_METHOD") ; }
/* script_filename ain't in the spec but seems to be implemented */
char * script_filename() { return (char*)getenv("SCRIPT_FILENAME") ; }
char * script_name() { return (char*)getenv("SCRIPT_NAME") ; }
char * server_admin() { return (char*)getenv("SERVER_ADMIN") ; }
char * server_name() { return (char*)getenv("SERVER_NAME") ; }
char * server_port() { return (char*)getenv("SERVER_PORT") ; }
char * server_protocol() { return (char*)getenv("SERVER_PROTOCOL") ; }
char * server_software() { return (char*)getenv("SERVER_SOFTWARE") ; }

/* 
=================================================================
Everything below here belongs in class HTTP_REQUEST, which will be a
parent of CGI as soon as I've fixed it up!
==================================================================== */

/*NOTE: authorization should NOT be passed to CGI unless the server is hacked
  CGI/1.1 is too old to know about proxy authorization, but I guess a server
  probably will keep the 'spirit' of it and NOT return it ...
*/
char * authorization() { return (char*)getenv("HTTP_AUTHORIZATION") ; }
char * proxy_authorization() { return (char*)getenv("HTTP_PROXY_AUTHORIZATION") ; }

/* The rest of the headers will probably be available to CGI whenever set
   (but I really don't know where to draw the line for authentication
   stuff - like MD5 - does anyone?   The CGI/1.1 spec simply isn't
   adequate for programming with HTTP/1.1).
*/

/* GENERAL HEADERS */
char * cache_control() { return (char*)getenv("HTTP_CACHE_CONTROL") ; }
char * connection() { return (char*)getenv("HTTP_CONNECTION") ; }
char * date() { return (char*)getenv("HTTP_DATE") ; }
char * pragma() { return (char*)getenv("HTTP_PRAGMA") ; }
char * transfer_encoding() { return (char*)getenv("HTTP_TRANSFER_ENCODING") ; }
char * upgrade() { return (char*)getenv("HTTP_UPGRADE") ; }
char * via() { return (char*)getenv("HTTP_VIA") ; }

/* REQUEST HEADERS */
/* char * accept() { return (char*)getenv("HTTP_ACCEPT") ; } */
char * accept_charset() { return (char*)getenv("HTTP_ACCEPT_CHARSET") ; }
char * accept_encoding() { return (char*)getenv("HTTP_ACCEPT_ENCODING") ; }
char * accept_language() { return (char*)getenv("HTTP_ACCEPT_LANGUAGE") ; }
char * cookie() { return (char*)getenv("HTTP_COOKIE") ; }
char * from() { return (char*)getenv("HTTP_FROM") ; }
char * host() { return (char*)getenv("HTTP_HOST") ; }
char * if_modified_since() { return (char*)getenv("HTTP_IF_MODIFIED_SINCE") ; }
char * if_match() { return (char*)getenv("HTTP_IF_MATCH") ; }
char * if_none_match() { return (char*)getenv("HTTP_IF_NONE_MATCH") ; }
char * if_range() { return (char*)getenv("HTTP_IF_RANGE") ; }
char * if_unmodified_since() { return (char*)getenv("HTTP_IF_UNMODIFIED_SINCE") ; }
char * max_forwards() { return (char*)getenv("HTTP_MAX_FORWARDS") ; }
char * range() { return (char*)getenv("HTTP_RANGE") ; }
char * referer() { return (char*)getenv("HTTP_REFERER") ; }
char * user_agent() { return (char*)getenv("HTTP_USER_AGENT") ; }

/* ENTITY HEADERS (the spec is rather awkward: content-type and length
//      are both CGI headers and Entity headers.   Can fix this with
//      the HTTP class hierarchy in a future release).
//
// Not that we expect to get most of these very often... */
char * allow() { return (char*)getenv("HTTP_ALLOW") ; }
char * content_base() { return (char*)getenv("HTTP_CONTENT_BASE") ; }
char * content_encoding() { return (char*)getenv("HTTP_CONTENT_ENCODING") ; }
char * content_language() { return (char*)getenv("HTTP_CONTENT_LANGUAGE") ; }
char * http_content_length() { return (char*)getenv("HTTP_CONTENT_LENGTH") ; }
char * content_location() { return (char*)getenv("HTTP_CONTENT_LOCATION") ; }
char * content_md5() { return (char*)getenv("HTTP_CONTENT_MD5") ; }
char * content_range() { return (char*)getenv("HTTP_CONTENT_RANGE") ; }
char * http_content_type() { return (char*)getenv("HTTP_CONTENT_TYPE") ; }
char * etag() { return (char*)getenv("HTTP_ETAG") ; }
char * expires() { return (char*)getenv("HTTP_EXPIRES") ; }
char * last_modified() { return (char*)getenv("HTTP_LAST_MODIFIED") ; }



/* Smart implementation of the HTTP Request wants its own class, which will
   have to wait for a future release of CGI++.   Here's a token gesture...
   headers which either do or don't include a word (boolean), and
   a version of cookie to process KEY=VALUE pairs.
   Of course, the smart version of QUERY_STRING is already dealt with by FORM :-
)
*/

/* Issue an authentication challenge for this document */

void authenticate(char* authtype,char* realm);


void decode(char *s) 
{ 	char *nextparam;
	char *p;
	STRING *name, *value;
	int hasmore=1;
	if(s==NULL) return;
	while(hasmore) {
		if(*s==0) break;
		nextparam=(char*)strchr(s,'&');
		if(nextparam!=NULL) {
			hasmore=1;
			*nextparam=0;
			nextparam++;
		}
		else hasmore=0;
		p = (char*)strchr(s,'=');
		if(p!=NULL) {
			*p=0;
			p++;
		}
		name = www_unescape(s);
		
		if(p!=NULL) value = www_unescape(p);
		else value = PCharToString("");
	
		AddToTail(NewParam(name,value));	

		s = nextparam;	
	}	
}

STRING * key_val(char *content, char *key)
{	STRING *t;
	char buf[MAXLEN];	
	char *p;
	if(strlen(key)>MAXLEN-100) ErrorExit(TOOLONGKEY);
	if(content==NULL) return NULL;
	snprintf(buf,MAXLEN,"%s=",key);	
	p = (char *)strstr(content,buf);
	if(p==NULL)return NULL;
	t = StringNew();
	p+=strlen(buf);
	while(*p && *p!=';') {
		StringAddChar(t,*p); 
		p++;
	}
	return t;
}

STRING * cookie_val(char *key)
{	
	return key_val(cookie(),key);
}

char * memstr(char *mem, int len, char *key, int keylen)
{	int i;
	if(mem==NULL || key==NULL) return NULL;
	if(len<keylen) return NULL;
	if(len<=0) return NULL;
	if(keylen<=0) return mem;
	for(i=0;i<len - keylen;i++) {
		if(memcmp(mem+i,key,keylen)==0) return mem+i;
	}
	return NULL;
}

void read_mime_body(char *body, int len, char * boundary) 
{ 
	char *p;
	char *data,*dataend;
	char *bodyend;
	STRING * startboundary, *endboundary;
	int startboundarylen,endboundarylen;
	char *startsection, * head, *name, *filename=NULL;
	int isfile;
	char *cd = "Content-Disposition: ";
	if(len<=0) return ;
	bodyend = body+len;

	startboundary = StringNew();
	endboundary = StringNew();
	StringCatPChar(startboundary,boundary);
	StringCatPChar(startboundary,"\r\n");
	StringCatPChar(endboundary,"\r\n--");
	StringCatPChar(endboundary,boundary);
	startboundarylen = strlen(GetPChar(startboundary));
	endboundarylen = strlen(GetPChar(endboundary));
	while(1) {
		len = bodyend - body;
		startsection = memstr(body,len,GetPChar(startboundary),
			startboundarylen);
		if(startsection==NULL) break; 
		startsection += startboundarylen;
		body = startsection;
		head = (char *)strstr(startsection,cd);
		if(head==NULL) continue;
		head += strlen(cd);
		head = (char*)strstr(head,"name=\"");
		if(head==NULL) continue;
		head += strlen("name=\"");
		name = head;
		while(*head && *head!='"') head++;
		if( *head!='"') continue;
		*head = 0;
		head++;
		isfile = 0;
		dataend = memstr(head,bodyend-head,GetPChar(endboundary),endboundarylen);
		if(dataend ==NULL) continue;
		*dataend = 0;
		if( (p=(char*)strstr(head,"filename=\""))!=NULL) {
			isfile = 1;
			p+=strlen("filename=\"");
			filename = p;
			while(*p && *p!='"')p++;
			if(*p!='"') continue;
			*p=0;
			p++;
			head = p;
		}
		data = (char *)strstr(head,"\r\n\r\n");
		if(data==NULL) continue;
		data += 4;
		if(isfile) {
			STRING *sname, *sfilename;
			UPLDFILE *cgif;
			sname = ((*name)==0) ? PCharToString("") : PCharToString(name);
			sfilename = (*filename==0) ? PCharToString("") : PCharToString(filename);
			cgif = NewUPLDFile(sname,sfilename,dataend-data,data);
			AddToUPLDFileTail(cgif);
		}
		else {
			STRING *sname, *value;
			sname = (*name==0) ? PCharToString("") : PCharToString(name);
			value = (data==dataend) ? PCharToString("") : PCharToString(data);
			AddToTail(NewParam(sname,value));
		}
		body = dataend+1;
	}
	StringDestroy(startboundary);	
	StringDestroy(endboundary);	
} 

void InitCGI(void)
{
	char * clen = content_length();
	int len =  clen == NULL ? 0 : atoi(clen) ;
  	char * ctype = content_type() ;
  	char * method = request_method() ;

  	if ( method == NULL ) return;

/* GET or HEAD will send us form data in here.   The new HTTP/1.1 methods
   could in principle also do so.   We'll decode whatever we get.
*/
  	decode(query_string()) ;

/* POST and PUT require a body (for other methods, we'll take one if
   we're given one, but not insist :-)

   CGI/1.1 guarantees us a valid Content-length.   However, so does
   HTTP/1.0, for which CGI/1.1 was really written.   HTTP/1.1 doesn't,
   so we might find someone sending us a body WITHOUT a content-length
   (in violation of the spec).   If so, this will break.
*/
  	if(clen==NULL)
    		if(strcasecmp(method,"PUT")==0 || strcasecmp(method,"POST")==0)
			ErrorExit(NOCONTENTLEN);
  	if ( len > 0 ) {
		int bodylen;
		char * body;

    		bodylen = len+1;
    		body = (char*)malloc(bodylen) ;
		if(body==NULL) ErrorExit(MEMERROR);
    		fread(body,1,len,stdin) ;

    		body[len] = 0 ;
    		if (strcasecmp(ctype,"application/x-www-form-urlencoded")==0){
        		decode(body) ;
			free(body);
        		body = NULL ;
    		} else if ( (char*)strstr(ctype,"multipart/form-data")!=NULL){
			STRING *t;
			t=key_val(ctype,"boundary");
        		read_mime_body(body,len,GetPChar(t)) ; 
			StringDestroy(t);
			free(body);
        		body = NULL ;
    		} else {
/* Got a BODY of unknown type: might be an error.   Let the programmer
   have it in its raw form (i.e. do nothing - keep it in _body).
*/
        		ErrorExit(ERRORTYPE);
    		}
	}
}

int CGImain(void);

int main()
{
	InitCGI();	
	return CGImain();
}


void DUMP()
{
	PARAM *p;
	UPLDFILE *upf;
	p = GetFirstParam();
	while(p) {
		printf(":%s:=:%s<br>\n",GetParamName(p),GetParamValue(p));
		p = GetNextParam();
	}
	upf = GetFirstUPLDFile();
	while(upf) {
		printf("File: name=:%s: filename=:%s: filelen=%ld<br>",
			GetUPLDParamName(upf),
			GetUPLDFileName(upf),
			GetUPLDFileLen(upf));
		upf = GetNextUPLDFile();
	}
	printf("<p>\n");
}
/*
CGImain()
{	char *p;
	int len;
	UPLDFILE *f;
	f = GetUPLDFileByName("aaa");
	if(f==NULL) { printf("Content-type:text/html\n\nError!\n");
		exit(0);
	}
	len = GetUPLDFileLen(f);
	p = GetUPLDFileData(f);
	printf("Content-type: audio/x-mpeg\n\n");
	fwrite(p,1,len,stdout);
	return 0;
}

*/
