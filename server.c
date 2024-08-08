#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include "record.h"

void handle_client(int cfd, FILE *f) {
    char received[100];
    char actual[31];
    unsigned short y;
    ssize_t read_one = 0;
    char none[11] = "none\n";
    size_t buffer_len = 0;

    while (1) {
        y = 0;
        char send[11];
        memset(send, 0, sizeof(send));
        memset(actual, 0, sizeof(actual));
        read_one = read(cfd, received + buffer_len, sizeof(received) - buffer_len - 1);
        if (read_one == -1) {
            perror("badclient");
            break;
        }
        if (read_one == 0) {
            fprintf(stderr, "bad client");
            break;
        }

        buffer_len += read_one;
        received[buffer_len] = '\0';

       
        size_t processed_len = 0;
        while (processed_len < buffer_len) {
            char *newline_pos = strchr(received + processed_len, '\n');
            if (!newline_pos) {
                break;
            }

            size_t message_len = newline_pos - (received + processed_len) + 1;
            if (message_len > sizeof(actual) - 1) {
                fprintf(stderr, "Message too long");
                processed_len += message_len;
                continue;
            }

            strncpy(actual, received + processed_len, message_len);
            actual[message_len - 1] = '\0';
            processed_len += message_len;

            if (f == NULL || fseek(f, 0, SEEK_SET) != 0) {
                break;
            }

            record r;
            int found = 0;
            while (fread(&r, sizeof(record), 1, f) == 1) {
                if (strlen(actual) == r.name_len && strncmp(actual, r.name, r.name_len) == 0) {
                    y = r.sunspots;
                    snprintf(send, sizeof(send), "%d\n", y);
                    found = 1;
                    break;
                }
            }

            if (!found) {
                strncpy(send, none, sizeof(send));
            }

            if (write(cfd, send, strlen(send)) == -1) {
                perror("failed to send");
                break;
            }
        }

        if (processed_len < buffer_len) {
            memmove(received, received + processed_len, buffer_len - processed_len);
            buffer_len -= processed_len;
        } else {
            buffer_len = 0;
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

