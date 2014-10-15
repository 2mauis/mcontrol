// mcontrol.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <Windows.h>
#include <WinSock.h>
#include <stdlib.h>
#pragma comment( lib, "ws2_32.lib" )
#define BUF_SIZE 4096

struct req_head{
	char method[7];
	char host[255];
	char path[255];
};

struct req_head parse_request_head(char *request);

// volume up
void volume_up();

// volume down
void volume_down();

// mute and unmute
void mute();

// next
void play_next();

// previous
void play_prev();

// play and pause
void play();

// start and stop
void start();

int load_file(char *filename, char *content);

void run();

int _tmain(int argc, _TCHAR* argv[])
{
	run();

	return 1;
}

int load_file(char *file_name, char *content)
{
	FILE *file;
	if (fopen_s(&file, file_name, "rb+") != 0)
	{
		return -1;
	}

	int file_size;

	// set cursor to the end of the file
	fseek(file, 0L, SEEK_END);

	// get file size
	file_size = ftell(file);

	// reset file cursor
	rewind(file);

	// set memory
	memset(content, 0, file_size + 1);

	// load file data to content
	fread(content, sizeof(char), file_size, file);

	// close the stream
	fclose(file);

	// set file end
	content[file_size] = 0;

	return file_size;
}


void run()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Init failed");
		exit(-1);
	}

	SOCKET server_fd;

	server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (server_fd == INVALID_SOCKET)
	{
		printf("Invalid socket");
		exit(-1);
	}

	struct sockaddr_in server_addr;
	int port = 56789;
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
	{
		printf("Socket bind error");
		exit(-1);
	}

	if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR)
	{
		printf("Socket execute error");
		exit(-1);
	}

	char hostname[255];
	char *ip;
	char *local = "*";
	struct hostent *hostinfo;
	if (gethostname(hostname, sizeof(hostname)) == 0)
	{
		if ((hostinfo = gethostbyname(hostname)) != NULL)
		{
			ip = inet_ntoa(*(struct in_addr *)*hostinfo->h_addr_list);
		}
		else
		{			
			ip = local;
		}
	}
	else
	{
		ip = local;
	}
	printf("Server is listening on %s:%d\n", ip, port);
	char doccmd[BUF_SIZE];
	sprintf_s(doccmd, BUF_SIZE, "start http://%s:%d/html/help.html", ip, port);
	system(doccmd);

	while (true)
	{
		SOCKET client;
		struct sockaddr_in client_addr;
		memset(&client_addr, 0, sizeof(struct sockaddr_in));
		client = accept(server_fd, (struct sockaddr*)&client_addr, 0);
		if (client == INVALID_SOCKET)
		{
			printf("Invalid socket");
		}
		//printf("Access success\n");

		char request[BUF_SIZE];
		char *token, *p;
		char *content;
		content = (char *)malloc(sizeof(char)* BUF_SIZE);
		char htmlfile[255];

		token = (char *)malloc(sizeof(char)* 255);
		p = (char *)malloc(sizeof(char)* 255);

		int receive_rs = recv(client, request, sizeof(request), 0);
		parse_request_head(request);
		//printf("%s\n", request);
	
		token = strtok_s(request, " ", &p);
		token = strtok_s(NULL, " ", &p);
		//printf("%s\n", token);

		if (token != NULL)
		{
			if (strcmp(token, "/next") == 0)
			{
				play_next();
			}
			else if (strcmp(token, "/prev") == 0)
			{
				play_prev();
			}
			else if (strcmp(token, "/play") == 0)
			{
				play();
			}
			else if (strcmp(token, "/up") == 0)
			{
				volume_up();
			}
			else if (strcmp(token, "/down") == 0)
			{
				volume_down();
			}
			else if (strcmp(token, "/mute") == 0)
			{
				mute();
			}

			strcpy_s(htmlfile, token);
		}
		else
		{
			char *index = "html/index.html";
			strcpy_s(htmlfile, index);
		}

		// 加载要显示的页面
		char *html;
		html = (char *)malloc(sizeof(char)* BUF_SIZE);
		int size_html = load_file(htmlfile, html);
		//printf("Loading file: %s\n", token);

		if (size_html == -1)
		{
			sprintf_s(content, BUF_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", BUF_SIZE, "<h1>404 NOT FOUND</h1>");
		}
		else
		{
			sprintf_s(content, BUF_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", size_html, html);
		}
		receive_rs = send(client, content, strlen(content), 0);
		free(content);
		free(html);

		closesocket(client);
	}

	closesocket(server_fd);
}

struct req_head parse_request_head(char *request)
{
	struct req_head head;
	memset(&head, 0, sizeof(head));

	enum {
		method_start = 1,
		method_end,
		path_start,
		path_end,
		ver_start,
		ver_end,
		host_start,
		host_end
	} status;

	status = method_start;

	int i = 0, j = 0;
	for (;;)
	{
		//printf("%c", request[i]);
		if (request[i] == '\r' && request[i + 1] == '\n' && request[i + 2] == '\r' && request[i + 3] == '\n')
		{
			//request[i+2] = '\0';
			break;
		}

		switch (status)
		{
		case method_start:
			if (request[i] != ' ')
			{
				head.method[j++] = request[i];
			}
			else
			{
				j = 0;
				status = method_end;
			}
			break;

		case method_end:
			if (request[i] != ' ')
			{
				head.path[j++] = request[i];
				status = path_start;
			}
		}
		i++;
	}
	
	printf("%s\n", head.method);
	return head;
}

void volume_up()
{
	keybd_event(VK_VOLUME_UP, 0, 0, 0);
	keybd_event(VK_VOLUME_UP, 0, KEYEVENTF_KEYUP, 0);
}

void volume_down()
{
	keybd_event(VK_VOLUME_DOWN, 0, 0, 0);
	keybd_event(VK_VOLUME_DOWN, 0, KEYEVENTF_KEYUP, 0);
}

void mute()
{
	keybd_event(VK_VOLUME_MUTE, 0, 0, 0);
	keybd_event(VK_VOLUME_MUTE, 0, KEYEVENTF_KEYUP, 0);
}

void play()
{
	keybd_event(VK_MEDIA_PLAY_PAUSE, 0, 0, 0);
	keybd_event(VK_MEDIA_PLAY_PAUSE, 0, KEYEVENTF_KEYUP, 0);
}

void play_next()
{
	keybd_event(VK_MEDIA_NEXT_TRACK, 0, 0, 0);
	keybd_event(VK_MEDIA_NEXT_TRACK, 0, KEYEVENTF_KEYUP, 0);
}

void play_prev()
{
	keybd_event(VK_MEDIA_PREV_TRACK, 0, 0, 0);
	keybd_event(VK_MEDIA_PREV_TRACK, 0, KEYEVENTF_KEYUP, 0);
}

void start()
{
	keybd_event(VK_MEDIA_STOP, 0, 0, 0);
	keybd_event(VK_MEDIA_STOP, 0, KEYEVENTF_KEYUP, 0);
}