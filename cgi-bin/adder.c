#include "csapp.h"

int main(void){
	char *buf, *p;
	char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
	int n1=0, n2=0;

	if((buf = getenv("QUERY_STRING")) != NULL){
		p = strchr(buf, '&');
		*p = '\0';
		strcpy(arg1, buf);
		strcpy(arg2, p+1);
		n1 = atoi(arg1);
		n2 = atoi(arg2);
	}

	// Response body
	sprintf(content, "QUERY_STRING=%s", buf);
	sprintf(content, "This is hn13's demo: ");
	sprintf(content, "%sThe Internet addition portal.\r\n<p>", content);
	sprintf(content, "%sThe answer is: %d+%d=%d",content, n1, n2, n1+n2);
	sprintf(content, "%sThanks for visting!", content);

	// Response header
	printf("Connection: Close\r\n");
	printf("Content-length: %d\r\n", (int)strlen(content));
	printf("Content-type: text/html\r\n\r\n");
	printf("%s",content);
	fflush(stdout);
}