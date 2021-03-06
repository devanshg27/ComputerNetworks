#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <linux/limits.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>

#define PORT 8080
char file_name[PATH_MAX + 10], buffer[1024 + 10];

using namespace std;

int main(int argc, char const *argv[]) {
	int server_fd;
	struct sockaddr_in address;

	int opt = 1;
	int addrlen = sizeof(address);

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(PORT);
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(address.sin_zero), '\0', 8);

	if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if(listen(server_fd, 10) < 0) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}

	int new_socket, valread;

	while(1) {
		if((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			perror("accept failed");
			continue;
		}

		int noOfFiles = 0;

		while(true) {
			int t = read(new_socket, buffer, 1);
			if(t == -1){
				perror("read failed");
				noOfFiles = 0;
				break;
			}
			else if(buffer[0] == '\n') break;
			else noOfFiles = (noOfFiles * 10) + buffer[0] - '0'; 
		}

		for(int i=0; i<noOfFiles; ++i) {
			int len = 0;
			bool noerror = true;
			file_name[len] = '\0';
			while(true){
				int t = read(new_socket, buffer, 1);
				if(t == -1){
					perror("read failed");
					noerror = false;
					break;
				}
				else if(buffer[0] == '\n') break;
				else {
					file_name[len++] = buffer[0];
					file_name[len] = '\0';
				}
			}

			if(!noerror) continue;

			int fd = open(file_name, O_RDONLY);
			if(fd < 0) {
				perror("open failed");
				string str = to_string(-1) + '\n';
				send(new_socket , str.c_str() , strlen(str.c_str()) , 0);
				continue;
			}

			struct stat st;
			if(stat(file_name, &st) < 0) {
				perror("stat failed");
				string str = to_string(-1) + '\n';
				send(new_socket , str.c_str() , strlen(str.c_str()) , 0);
				close(fd);
				continue;
			}

			string str = to_string(st.st_size) + '\n';
			send(new_socket , str.c_str() , strlen(str.c_str()) , 0);

			while (noerror) {
				int bytes_left = read(fd, buffer, 1024);
				if(bytes_left == 0) {
					break;
				}
				if(bytes_left < 0) {
					perror("read failed");
					noerror = false;
					break;
				}

				char* p = buffer;
				while(bytes_left > 0) {
					int bytes_written = write(new_socket, p, bytes_left);
					if (bytes_written <= 0) {
						perror("write failed");
						noerror = false;
						break;
					}
					bytes_left -= bytes_written;
					p += bytes_written;
				}
			}
			close(fd);
		}
		close(new_socket);
	}

	close(server_fd);
	return 0;
}