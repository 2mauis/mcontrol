// mcontrol.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"

#include <Windows.h>
#include <WinSock.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "mime_types.h"
#include "mime_extensions.h"

#pragma comment( lib, "ws2_32.lib" )
#define BUF_SIZE 4096

struct req_head{
	char method[7];
	float version;
	char host[255];
	size_t port;
	char path[255];
};

struct fbuf{
	char *buf;
	size_t buf_size;
	char *mime;
};

struct req_head parse_request_head(char *request);
void strcpyn(char *dest, const char *source, int start, int length);
char *get_mime(const char *filename);

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

struct fbuf *load_file(char *filename);

void run();

int _tmain(int argc, _TCHAR* argv[])
{
	run();

	return 1;
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
	sprintf_s(doccmd, BUF_SIZE, "start http://%s:%d/", ip, port);
	//system(doccmd);

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

		char request[BUF_SIZE];
		char htmlfile[255];

		int receive_rs = recv(client, request, sizeof(request), 0);
		struct req_head head = parse_request_head(request);

		if (strcmp(head.path, "/next") == 0)
		{
			play_next();
		}
		else if (strcmp(head.path, "/prev") == 0)
		{
			play_prev();
		}
		else if (strcmp(head.path, "/play") == 0)
		{
			play();
		}
		else if (strcmp(head.path, "/up") == 0)
		{
			volume_up();
		}
		else if (strcmp(head.path, "/down") == 0)
		{
			volume_down();
		}
		else if (strcmp(head.path, "/mute") == 0)
		{
			mute();
		}
		else if (strcmp(head.path, "/") == 0)
		{
			strcpy_s(htmlfile, "html/index.html");
		}	
		else
		{
			strcpyn(htmlfile, head.path, 1, sizeof(head) - 1);
		}

		// 加载要显示的页面
		struct fbuf *html = load_file(htmlfile);
		printf("Loading file: %s\n", htmlfile);

		char *content;
		if (html == NULL)
		{
			content = (char *)calloc(BUF_SIZE, sizeof(char));
			sprintf_s(content, BUF_SIZE, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", 22, "<h1>404 NOT FOUND</h1>");
		}
		else
		{
			size_t total_size = html->buf_size + 67;
			content = (char *)calloc(total_size, sizeof(char));
			sprintf_s(content, total_size, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n%s", html->mime, strlen(html->buf), html->buf);
		}
		receive_rs = send(client, content, strlen(content), 0);
		free(content);
		free(html);

		closesocket(client);
	}

	closesocket(server_fd);
}

struct fbuf *load_file(char *filename)
{
	struct _stat sbuf;
	if (_stat(filename, &sbuf) == -1)
	{
		return NULL;
	}

	struct fbuf *ct;
	if ((ct = (struct fbuf *)malloc(sizeof(struct fbuf))) == NULL)
	{
		return NULL;
	}
	ct->buf_size = sbuf.st_size;
	ct->buf = (char *)calloc(ct->buf_size, sizeof(char));
	ct->mime = (char *)calloc(24, sizeof(char));

	FILE *fd;
	if (fopen_s(&fd, filename, "r") != 0)
	{
		return NULL;
	}

	// load file data to content
	fread(ct->buf, sizeof(char), ct->buf_size, fd);
	ct->buf[ct->buf_size] = '\0';
	
	strcpy_s(ct->mime, sizeof(char)*24, get_mime(filename));

	// close the stream
	fclose(fd);

	return ct;
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
		host_end,

		port_start,
		port_end,

		head_end,
	} status;

	status = method_start;

	int i = 0, j = 0;
	char http_version_str[7], port_str[7];
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
				head.method[j] = '\0';
				j = 0;
				status = path_start;
			}
			break;

		case method_end:
				
			break;

		case path_start:
			if (request[i] != ' ')
			{
				head.path[j++] = request[i];
			}
			else
			{
				head.path[j] = '\0';
				j = 0;
				status = path_end;
			}
			break;

		case path_end:

			if (toupper(request[i]) == 'H' && toupper(request[i + 1]) == 'T' && toupper(request[i + 2]) == 'T' && toupper(request[i + 3]) == 'P')
			{
				i += 4;
				status = ver_start;
			}
			break;

		case ver_start:

			if (request[i] == '\r' || request[i] == '\n')
			{
				http_version_str[j] = '\0';
				j = 0;
				status = ver_end;
			}
			else
			{
				http_version_str[j++] = request[i];
			}
			break;
		
		case ver_end:
			head.version = atof(http_version_str);
			if (head.version > 1.0)
			{
				if (toupper(request[i]) == 'H' && toupper(request[i + 1]) == 'O' && toupper(request[i + 2]) == 'S' && toupper(request[i + 3]) == 'T')
				{
					i += 4;
					status = host_start;
				}
			}
			else
			{
				status = head_end;
			}
			break;

		case host_start:

			if (request[i] == ':')
			{
				head.host[j] = '\0';
				j = 0;
				status = port_start;
			}
			else if (request[i] != ' ')
			{
				head.host[j++] = request[i];
			}
				
			break;
			
		case host_end:

			break;

		case port_start:	
			if (request[i] == '\r' || request[i] == '\n')
			{
				port_str[j] = '\0';
				j = 0;
				status = port_end;
			}
			else
			{
				port_str[j++] = request[i];
			}
			break;

		case port_end:
			head.port = atoi(port_str);
			break;
		}
		i++;
	}
	
	return head;
}

void strcpyn(char *dest, const char *src, int start, int len)
{
	for (int i = 0, j = start; j <= len; i++, j++)
	{
		if (src[j] == '\0')
		{
			dest[i] = '\0';
			break;
		}
		else
		{
			dest[i] = src[j];
		}
		
	}
}

char *get_mime(const char *file)
{
	char *mime = (char *)calloc(24, sizeof(char));
	char *ext = (char *)calloc(10, sizeof(char));
	strcpy_s(ext, sizeof(char)*10, strrchr(file, '.'));
	strcpyn(ext, strrchr(file, '.'), 1, 10);
	printf("ext: %s\n", ext);

	for (int i = 0; i < MNUM_MIME_EXTENSIONS; i++) {
		if (strcmp(ext, mime_extensions[i]) == 0)
		{
			if (mime == NULL) return NULL;
			strcpy_s(mime, sizeof(char)*24, mime_types[i]);
			printf("mime: %s\n", mime);
		}
	}
	return mime;
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