#include "basic.h"
#define HTabSize 571

char8 leftQuoteStr[] = "“〔（《【「『";
char8 rightQuoteStr[] = "”〕」）》】』";
char8 CDelimStr[] = "∥├◤◇│↘◢█◆‧※◎　■★•→＃％［］＼：＜＞－＊＋（）●＆｜～▲？，。；：／、！．";
char8 AllDelim[256];
char8 *DelimPat[] = {
	", ", "- ", "ft.", "feat.", " (", ") "};

char8 in_delims[] = "-+$@#&*'!._";
char8 ex_delims[] = " `~\t\n:;?,[]{}()|=\"";
char8 Delim1[] = "`~\t\n:;?,[]{}()|=\"";
char8 Delim3[] = "“〔（《【「『”〕」）》】』∥├◤◇│↘◢█◆‧※◎　■★•→＃％［］＼：＜＞－＊＋（）●＆｜～▲？，。；：／、！．▼";

int	DelimSym[256]; 
char8 *HTab[HTabSize]; 

int	isDelim(char8 *); 
char8 *getU8Ch(char8 *, char8 *);

char8 *HTab[HTabSize]; 

//	use a simple string polynomial function, where the radix
//	value is chosen to be 65 which can be computed by one shift plus one plus operation

uint hash65(char8 *pat)
{
	int32 hash;
	char8 *ptr;
	int	c; 

	ptr = pat;

	hash = 61; 
	while((c=(*ptr++))) {
		hash = c + (hash<<6) + hash;	// hash * 65 + *ptr; 
	}
	return hash; 
}

void DelimSymInit()
{
	int	i, c, len; 
	for(i=0; i<256; i++) {
		DelimSym[i] = 0; 
	}	
	for(i=224; i<256; i++) {
		DelimSym[i] = 1; 
	}
	len = strlen(Delim1);
	for(i=0; i<len; i++) {
		c = Delim1[i]; 
		DelimSym[c] = 1; 
	}
}

//	use Open Addressing HTab 

void insertH(char8 *Ch)
{
	int32	hv; 
	int32	i;
	hv = hash65(Ch) % HTabSize; 
	i = hv; 
	while(HTab[i]) {
		i = (i+1) % HTabSize; 
	}
	HTab[i] = strdup(Ch); 
}
	
int	findH(char8 *Ch)
{
	int32	hv; 
	int32	i;
	hv = hash65(Ch) % HTabSize; 
	i = hv; 
	while(HTab[i] != NULL) {
		if(strcmp(HTab[i], Ch) == 0) {
			return 1; 
		}
		else {
			i = (i+1) % HTabSize; 
		}
	}
	return 0; 
}

//	Chinese delimiter characters 

void Delim3Init()
{
	int	i,j; 
	char8 Ch[8], *ptr;  
	
	for(i=0; i<HTabSize; i++) {
		HTab[i] = NULL; 
	}
	ptr = Delim3; 
	while(ptr = getU8Ch(ptr, Ch)) {
		insertH(Ch); 
	}
}

int	isDelim3(char8 *Ch)
{
	if(findH(Ch))  return 1; 
	// if(strstr(Delim3, Ch)) return 1; 
	return 0; 
}

int	isDelim1(char8 *Ch)
{
	if(strstr(Delim1, Ch)) return 1; 
	return 0; 
}

int	isDelim(char8 *Ch)
{
	int	len = strlen(Ch); 
	if(len == 3) {
		return isDelim3(Ch); 
	}
	else	{
		return isDelim1(Ch); 
	}
}

void U8LenInit(int *u8Len)
{
	int	c; 
	for(c=0; c<256; c++) {
		if(c > 240) {
			u8Len[c] = 4; 
		}
		else if(c >= 224) {
			u8Len[c] = 3; 
		}
		else if(c >= 192) {
			u8Len[c] = 2; 
		}
		else if(c >= 128) {
			u8Len[c] = 0; 	// error code, don't get it
		}
		else {
			u8Len[c] = 1; 
		}
	}
}
		
//	given a UTF8 text string get a chararcter from head and store it in Ch[];
//	return the position after the extracted character
//	we assume that the UTF8 text string is a good text string without broken code

char8 *getU8Ch(char8 *text, char8 Ch[])
{
	char8 *ptr, *qtr; 
	if(*text == '\0') {
		Ch[0] = '\0'; 
		return NULL; 
	}
	ptr = text; 
	qtr = Ch;

	if(*ptr >= 224) {
		*qtr++ = *ptr++; 
		*qtr++ = *ptr++; 
		*qtr++ = *ptr++; 
		*qtr = '\0'; 
		return ptr; 
	}
	if(*ptr < 192) {
		*qtr++ = *ptr++; 
		*qtr = '\0'; 
		return ptr; 
	}
	else {
		*qtr++ = *ptr++; 
		*qtr++ = *ptr++; 
		*qtr = '\0'; 
		return ptr; 
	}
}

//	getUTerm get a UTF8 term and return the position after the extracted term
//	a term can be a CJK term or spelling-based term like English 
//	a term a consecutive characters delimitted by a delimiter, 
//	we will have two delimiter strings in isDelim()  function

char8 *getUTerm(char8 *text, char8 *term)
{
	char8 *ptr, *qtr; 
	char8 Ch[10]; 
	int	len; 
	qtr = term; 
	ptr = text; 

	// printf("text: %s\n", text); fflush(stdout);

	if(*ptr == '\0') {
		*term = '\0'; 
		return NULL; 
	}
	if(*ptr == ' ') {
		term[0] = ' '; 
		term[1] = '\0'; 
		return text+1; 
	}
	ptr = getU8Ch(ptr, Ch) ; 
	if(isDelim(Ch)) { 
		strcpy(term, Ch); 
		return ptr; 
	}
	len = strlen(Ch); 
	if(len == 3) {	// CJK
		*qtr++ = Ch[0]; 
		*qtr++ = Ch[1]; 
		*qtr++ = Ch[2]; 
		len = 3; 
		while(len == 3)  {
			ptr = getU8Ch(ptr, Ch); 
			len = strlen(Ch); 
			if(len < 3) {
				ptr = ptr - len; 
				*qtr = '\0'; 
				return ptr; 
			}
			if(isDelim(Ch))  {
				*qtr = '\0'; 
				ptr = ptr - len; 
				return ptr; 
			}
			*qtr++ = Ch[0]; 
			*qtr++ = Ch[1];
			*qtr++ = Ch[2]; 
		}
	}
	else {
		*qtr++ = Ch[0]; 
		len = 1; 
		if(Ch[1]) {
			*qtr++ = Ch[1]; 
			len++; 
		}
		while(DelimSym[*ptr] == 0) {
			*qtr++ = *ptr++; 
		}
		*qtr = '\0'; 
		return ptr; 

		while(len < 3) {
			ptr = getU8Ch(ptr, Ch); 
			len = strlen(Ch); 
			if(len == 3) {
				ptr = ptr - len; 
				*qtr = '\0'; 
				return ptr; 
			}
			if(isDelim(Ch)) {
				ptr = ptr - len; 
				*qtr = '\0'; 
				return ptr; 
			}
			*qtr++ = Ch[0]; 
			if(Ch[1]) {
				*qtr++ = Ch[1]; 
			}
		}
	}
}

int	getSubterms(char8 *text, char8 **subterms)
{
	int	i, j, k; 
	char8 *ptr, *qtr; 
	int	tdx=0; 
	char pt[] = ". "; 
	char *token;
	char *lastptr;

	// printf("oldText: %s\n", text);
	ptr = text;
	while(*ptr == '/' || *ptr == '.') ptr++;
	if(strchr(text, '/')) {
		token = strtok(ptr, "/") ; 
		while(token != NULL) {
			subterms[tdx] = token; tdx++; 
			token = strtok(NULL, "/") ; 
		}
		// printf("tdx: %d\n", tdx);
	}
	else {
		lastptr = ptr;
		while(ptr = strchr(ptr, '.')) {
			ptr++;
			if((*ptr == ' ') || (*ptr == '\0')) {
				*(ptr-1) = '\0';
				if(*ptr == ' ') { *ptr = '\0'; ptr++; }
				subterms[tdx] = lastptr;
				tdx++;
			}
			lastptr = ptr; 
		}
		if(*lastptr) { subterms[tdx] = lastptr; tdx++; }
		// printf("tdx: %d\n", tdx);
	}
	return tdx; 
}

void usage()
{
	printf("termSeg:	use delimiters to break strings into terms\n"); 
	printf("	read from stdin, output to stdout\n"); 
	exit(1); 
}

void main(int argc, char **argv)
{
	char8 line[MaxLine]; 
	int	len; 
	char8 term[MaxLine]; 
	char8 *ptr;
	char8 *subterms[256]; 
	int	cnt; 
	
	DelimSymInit();
	Delim3Init(); 

	if(argc == 2 && (strcmp(argv[1], "-h")==0) ) usage(); 

	while(fgets(line, MaxLine, stdin)) {
		ptr = line; 
		// printf("Line: %s", line); fflush(stdout); 
		while(ptr) {
			ptr = getUTerm(ptr, term); 
			if(ptr) {
				if(strlen(term) > 0 && !isspace(term[0])) printf("%s\n", term); 
			}
			if(*term < 224) {
				cnt = getSubterms(term, subterms); 
				if(cnt <= 1) continue; 
				for(int i = 0; i < cnt; i++)  {
					printf("%s\n", subterms[i]); 
				}
			}
		}
	}
}