#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "record.h"
void handle_client(int cfd, FILE *f) {
	char received[31];
	unsigned short y;
	ssize_t read_one = 0;
	char none[11] = "none\n";
	while (1){
		y = 0;
		ssize_t read_total = 0;
		char send[11];
		memset(received, 0, sizeof(received));
		while ((read_one = read(cfd, received + read_total, sizeof(received) - read_total -1)) > 0){
			read_total += read_one;
			if (strchr(received, '\n') != NULL) break;
		}
		if (read_one == -1){
			fprintf(stderr, "error from reading da client");
			break;
		}
		if (read_one == 0){
			fprintf(stderr, "nothing was sent");
			break;
		}
		if (strchr(received, '\n') == NULL){
			fprintf(stderr, "bad client");
			break;
		}
		if (f == NULL || fseek(f, 0 , SEEK_SET) != 0){
			break;
		}
		*strchr(received,'\n') = '\0';
		record r;
		int exist = 0;
		while (fread(&r, sizeof(record), 1, f) == 1){			
			if (strlen(received) == r.name_len && strncmp(received, r.name, strlen(received)) == 0){
				y = r.sunspots;
				sprintf(send, "%d\n", y);
				break;
			}
		}
		if (y == 0){
			strcpy(send,none);
		}
		if (write(cfd,send,strlen(send)) == -1){
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
	FILE *f =fopen(argv[2], "rb");
	if (f == NULL){
		perror("error opening file");
		return 1;
	}
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	memset( &a, 0, sizeof(struct sockaddr_in));
	a.sin_family = AF_INET;
	a.sin_port = htons(atoi(argv[1]));
	a.sin_addr.s_addr = htonl(INADDR_ANY);
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
