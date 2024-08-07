#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "record.h"

int get_sunspots(FILE *f, const char *name, unsigned short *psunspots)
{
  if (f == NULL || fseek(f, 0, SEEK_SET) != 0) return 0;
  record r;

  while (fread(&r, sizeof(record), 1, f) == 1){
    if (strlen(name) == r.name_len && strncmp(name, r.name, strlen(name)) == 0 ){
      *psunspots = r.sunspots;
      return 1;
    }
  }
  return 0;
}
void handle_client(int cfd, FILE *f){
	while (1){
		char received[30];
		unsigned short y;
		if (read(cfd, received, 30) == -1){
			perror("error reading client");
			break;
		}
		if (strchr(received, '\n') == NULL){
			perror("bad client");
			break;
		}
		if (!get_sunspots(f, received, &y)){
			perror("failed to get sunspots");
			break;
		}
		char send[10];
		sprintf(send," %d", y);
		if (write(cfd, send, 10) == -1){
			perror("failed to send");
			break;
		}

	}
	return;
}
int main(int argc, char **argv){
	if (argc < 2){
		fprintf(stderr, "not enough arg");
		return 1;
	}

	int sfd;
	struct sockaddr_in a;
	FILE *f =fopen(argv[2], "r");
	if (f == NULL){
		perror("error opening file");
		return 1;
	}

	memset( &a, 0, sizeof(struct sockaddr_in));
	a.sin_family = AF_INET;
	a.sin_port = atoi(argv[1]);
	a.sin_addr.s_addr = INADDR_ANY;
	if (bind(sfd, (struct sockaddr *) &a, sizeof(struct sockaddr_in)) == -1 ){
		fprintf(stderr, "address already in use"); //is this fprintf or perror?
		close(sfd); //maybe remove this later
		exit(1);
	}

	if (listen(sfd, 2) == -1){
		perror("listen failed");
		close(sfd);
		return 1;
	}
	for(;;){
		int cfd, n;
		struct sockaddr_in ca;
		socklen_t sinlen;
		char x;

		sinlen = sizeof(struct sockaddr_in);
		if ((cfd = accept(sfd, (struct sockaddr *) &ca, &sinlen)) == -1){
			perror("accept failed");
			close(cfd);
			continue;
		}
		if (!fork()){ //chil process on the inside 
			close(sfd);
			handle_client(cfd, f); //implement this function
			close(cfd);
			exit(0);
		}
		//parent code
		close(cfd);
	}
	close(sfd);
	return 0;
}
