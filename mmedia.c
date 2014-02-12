// mmedia.cpp : 定义控制台应用程序的入口点。
//

/**
* TODO:
* 1. 封装socket
* 2. 读文件
*
*/
#include "stdafx.h"

#include <Windows.h>
#include <WinSock.h>
#include <stdlib.h>
#pragma comment( lib, "ws2_32.lib" )
#define MAX_FILE_SIZE 10240

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

	SOCKET server_socket;
	server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (server_socket == INVALID_SOCKET)
	{
		printf("Invalid socket");
		exit(-1);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(56789);

	int bind_rs = 0;
	bind_rs = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (bind_rs == SOCKET_ERROR)
	{
		printf("Socket bind error");
		exit(-1);
	}

	int listen_rs = 0;
	listen_rs = listen(server_socket, SOMAXCONN);
	if (listen_rs == SOCKET_ERROR)
	{
		printf("Socket execute error");
		exit(-1);
	}

	printf("Server is listening on %s\n", "*:56789");

	SOCKET client;
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	client = accept(server_socket, (struct sockaddr*)&client_addr, 0);
	if (client == INVALID_SOCKET)
	{
		printf("Invalid socket");
	}

	while (true)
	{
		char request[4096];
		int receive_rs = recv(client, request, sizeof(request), 0);
		char *token, *p;
		token = (char *)malloc(sizeof(char)* 60);
		p = (char *)malloc(sizeof(char)* 60);
		token = strtok_s(request, " ", &p);
		token = strtok_s(NULL, " ", &p);
		printf("%s", token);

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
		}
		else
		{
			break;
		}
		char *file = "E:\\www\\git\\mmedia\\index.html";
		char *content;
		content = (char *)malloc(sizeof(char)* MAX_FILE_SIZE);
		int fs = load_file(file, content);
		//printf("\nfs: %d\nstrlen: %d\n", fs, strlen(content));
		//printf(content);
		/**/
		if (fs == -1)
		{
			printf("Unable to load file: %s", file);
		}
		else
		{
			char *response;
			response = (char *)malloc(sizeof(char)* MAX_FILE_SIZE);
			_snprintf(response, 10240, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", fs, content);
			//printf("%d", strlen(response));
			receive_rs = send(client, response, strlen(response), 0);
			free(response);
		}
		/**/

	}

	//free(content);
	closesocket(client);
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