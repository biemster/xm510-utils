// copied from https://programmer.help/blogs/a-simple-ftp-client-implemented-in-c-language.html

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <arpa/inet.h>


static int  m_socket_cmd;
static int  m_socket_data;
static char m_send_buffer[1024];
static char m_recv_buffer[1024];


int socket_create() {
	return socket(AF_INET, SOCK_STREAM, 0);
}

int socket_connect(int s, char *server_ip, int server_port) {
	struct sockaddr_in server;
	server.sin_addr.s_addr = inet_addr(server_ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);
	
	if(connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
		return -1;
	}

	return 1;
}

int socket_send(int s, char *msg, int len) {
	return write(s, msg, len);
}

int socket_recv(int s, char *buf, int len) {
	return recv(s, buf, len, 0);
}

void socket_close(int s) {
	close(s);
}


//Command Port, Send Command
static int ftp_send_command(char *cmd) {
	int ret;
	ret = socket_send(m_socket_cmd, cmd, (int)strlen(cmd));
	if(ret < 0) return 0;

	return 1;
}

//Command Port, Receive Answer
static int ftp_recv_respond(char *resp, int len) {
	int ret;
	int off;
	len -= 1;
	for(off=0; off<len; off+=ret) {
		ret = socket_recv(m_socket_cmd, &resp[off], 1);
		if(ret < 0) return 0;
		if(resp[off] == '\n') break;
	}
	resp[off+1] = 0;

	return atoi(resp);
}

//Set FTP server to passive mode and resolve data ports
static int ftp_enter_pasv(char *ipaddr, int *port) {
	int ret;
	char *find;
	int a,b,c,d;
	int pa,pb;
	ret = ftp_send_command("PASV\r\n");
	if(ret != 1) return 0;

	ret = ftp_recv_respond(m_recv_buffer, 1024);
	if(ret != 227) return 0;

	find = strrchr(m_recv_buffer, '(');
	sscanf(find, "(%d,%d,%d,%d,%d,%d)", &a, &b, &c, &d, &pa, &pb);
	sprintf(ipaddr, "%d.%d.%d.%d", a, b, c, d);
	*port = pa * 256 + pb;

	return 1;
}

//Download Files
int ftp_download(char *name, void *buf, int len) {
	int i;
	int ret;
	char ipaddr[16] = {0};
	int port;
    
	//Query data address
	ret = ftp_enter_pasv(ipaddr, &port);
	if(ret != 1) return 0;

	//Connect data ports
	ret = socket_connect(m_socket_data, ipaddr, port);
	if(ret != 1) return 0;

	//Ready to download
	sprintf(m_send_buffer, "RETR %s\r\n", name);
	ret = ftp_send_command(m_send_buffer);
	if(ret != 1) return 0;

	ret = ftp_recv_respond(m_recv_buffer, 1024);
	if(ret != 125 && ret != 150){
		socket_close(m_socket_data);
		printf("Got returncode %d while trying to download the file.\n", ret);
		return 0;
	}
	
	//Start downloading and the server will automatically close the connection after reading the data
	for(i=0; i<len; i+=ret)	{
		ret = socket_recv(m_socket_data, ((char *)buf) + i, len);
		if(ret < 0) break;
	}
	
	//Download complete
	socket_close(m_socket_data);
	ret = ftp_recv_respond(m_recv_buffer, 1024);

	return (ret==226);
}

//Return file size
int ftp_filesize(char *name)
{
	int ret;
	int size;
	sprintf(m_send_buffer,"SIZE %s\r\n",name);
	ret = ftp_send_command(m_send_buffer);
	if(ret != 1) return 0;

	ret = ftp_recv_respond(m_recv_buffer, 1024);
	if(ret != 213) return 0;

	size = atoi(m_recv_buffer + 4);
	return size;
}

//Logon Server
int ftp_login(char *addr, int port, char *username, char *password) {
	int ret;
	ret = socket_connect(m_socket_cmd, addr, port);
	if(ret != 1) return 0;

    //Waiting for Welcome Message
	ret = ftp_recv_respond(m_recv_buffer, 1024);
	if(ret != 220) {
		socket_close(m_socket_cmd);
		return 0;
	}
	
    //Send USER
	sprintf(m_send_buffer, "USER %s\r\n", username);
	ret = ftp_send_command(m_send_buffer);
	if(ret != 1) 	{
		socket_close(m_socket_cmd);
		return 0;
	}
	ret = ftp_recv_respond(m_recv_buffer, 1024);
	if(ret != 331) {
		socket_close(m_socket_cmd);
		return 0;
	}
	
    //Send PASS
	sprintf(m_send_buffer, "PASS %s\r\n", password);
	ret = ftp_send_command(m_send_buffer);
	if(ret != 1) {
		socket_close(m_socket_cmd);
		return 0;
	}
	ret = ftp_recv_respond(m_recv_buffer, 1024);
	if(ret != 230) {
		socket_close(m_socket_cmd);
		return 0;
	}
	
    //Set to binary mode
	ret = ftp_send_command("TYPE I\r\n");
	if(ret != 1) {
		socket_close(m_socket_cmd);
		return 0;
	}
	ret = ftp_recv_respond(m_recv_buffer, 1024);
	if(ret != 200) {
		socket_close(m_socket_cmd);
		return 0;
	}

	return 1;
}

void ftp_quit() {
	ftp_send_command("QUIT\r\n");
	socket_close(m_socket_cmd);
}

void ftp_init() {
	m_socket_cmd = socket_create();
	m_socket_data= socket_create();
}

int main(int argc, char **argv) {
	if(argc == 4) {
		char server_ip[16] = {0};
		strncpy(server_ip, argv[1], 15);
		int server_port = 2121;

		ftp_init();
		if(ftp_login(server_ip, server_port, "anonymous", "anonymous")) {
			char filename[100] = {0};
			strncpy(filename, argv[3], 99);
			int fsize = ftp_filesize(filename);
			if(fsize > 0) {
				printf("File size: %d.\n", fsize);
				char *buf = (char*)malloc(fsize);
				if(ftp_download(filename, buf, fsize)) {
					int file_desc = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666);
					write(file_desc, buf, fsize);
					close(file_desc);
				}
				else {
					printf("Something went wrong, check lines above for error messages.\n");
				}
			}
			else {
				printf("File %s not found.\n", filename);
			}

			ftp_quit();
		}
		else {
			printf("Could not connect / login.\n");
		}
	}
	else {
		printf("Usage: %s <server ip> <server port> <filename>\n", argv[0]);
		return -1;
	}

	return 0;
}
