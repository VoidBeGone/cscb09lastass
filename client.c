#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#define MAX_SIZE 30

int main(int argc, char **argv){
	if (argc < 2){
		fprintf(stderr, "Need server Ip address");
		return 1;
	}

	int cfd;
	struct sockaddr_in a;

	memset(&a, 0, sizeof(struct sockaddr_in));
	a.sin_family = AF_INET;
	a.sin_port=htons(atoi(argv[2]));
	if (inet_pton(AF_INET, argv[1], &a.sin_addr) == 0){
		fprintf(stderr, "That is not a valid ipv4 address");
		return 1;
	}

	cfd = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(cfd, (struct sockaddr *) &a, sizeof(struct sockaddr_in)) == -1) {
		perror("failed to connect"); // is this fprintf or is this perror
		close(cfd);
		exit(1);
	}

	//this means that connection is sucessful 
	printf("Ready\n"); //is this the right printf f
	char send[MAX_SIZE];
	char response[11];
	ssize_t num_receive = 0;
	while (1){
		fflush(stdout);
		if (fgets(send,sizeof(send), stdin) == NULL){
			if (feof(stdin)){
				fprintf(stderr, "got eof");
				close(cfd);
				exit(1);
			}
			else{
				fprintf(stderr,"random error");
				close(cfd);
				exit(1);
			}
		}
		if (send[0] == '\n'){
			close(cfd);
			exit(0);
		}
		if (write(cfd, &send, strlen(send)) <= 0){
			perror("Error sending");
			close(cfd);
			exit(1);
		}
		memset(response, 0, MAX_SIZE);
		num_receive = 0;
		ssize_t total_read = 0;

		while ((num_receive = read(cfd, response + total_read, 11 -total_read -1)) > 0){
			total_read += num_receive;
			if (strchr(response, '\n') != NULL){
				break;
			}
		}
		if (num_receive == -1){
				perror("Error on receiving");
				close(cfd);
				exit(1);
		}
		if (num_receive == 0){
				fprintf(stderr, "no byte received");
				close(cfd);
				exit(1);
		}

		if (strchr(response, '\n') == NULL){
			fprintf(stderr, "ERROR FROM SERVER SENDING");
			close(cfd);
			exit(1);
		}
		response[10] = '\0';
		printf("%s",response);
	}
	close(cfd);
	return 0;
}
