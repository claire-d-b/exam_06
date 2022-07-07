#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

fd_set curr_sock, cpy_read, cpy_write;
char msg[42];
char buffer[1001];
char *send_msg = NULL;
char *str = NULL;
char *strad = NULL;
char *bufad = NULL;
int idz = -1;
typedef struct s_client
{
	int id;
	int fd;
	char *str;
	struct s_client *next;
} t_client;
t_client *client = NULL;

void write_msg(int fd, char *message)
{
	write(fd, message, strlen(message));
}

int extract_message(char **buf, char **msg)
{
	char *newbuf;
	int i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char *newbuf;
	int len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	printf("new_buf len: %lu\n", (sizeof(*newbuf) * (len + strlen(add) + 1)));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

int add_client(int fd, int sockfd)
{
	t_client *new = NULL;
	if (!(new = malloc(sizeof(t_client))))
	{
		write_msg(2, "Fatal error\n");
		close(sockfd);
		exit(1);
	}
	new->id = ++idz;
	new->fd = fd;
	new->str = NULL;
	if (!(new->str = malloc(sizeof(char))))
	{
		write_msg(2, "Fatal error\n");
		close(sockfd);
		exit(1);
	}
	bzero(new->str, sizeof(char));
	new->next = NULL;
	if (!client)
		client = new;
	else
	{
		t_client *tmp = client;
		while (tmp && tmp->next)
			tmp = tmp->next;
		tmp->next = new;
	}
	return idz;
}

int rm_client(int fd)
{
	t_client *rm = NULL;
	t_client *tmp = client;
	int id = 0;
	if (tmp && tmp->fd == fd)
	{
		rm = tmp;
		id = rm->id;
		client = client->next;
		free(rm);
		rm = NULL;
	}
	else
	{
		while (tmp && tmp->next && tmp->next->fd != fd)
			tmp = tmp->next;
		rm = tmp->next;
		id = rm->id;
		tmp->next = tmp->next->next;
		free(rm);
		rm = NULL;
	}
	return id;
}

int get_id(int fd)
{
	t_client *tmp = client;
	while (tmp)
	{
		if (tmp->fd == fd)
			return tmp->id;
		tmp = tmp->next;
	}
	return 0;
}

int get_max_fd(int sockfd)
{
	int max = sockfd;
	t_client *tmp = client;
	while (tmp)
	{
		if (tmp->fd > max)
			max = tmp->fd;
		tmp = tmp->next;
	}
	return max;
}

void send_all(int fd, char *message)
{
	t_client *tmp = client;
	while (tmp)
	{
		if (tmp->fd != fd && FD_ISSET(tmp->fd, &cpy_write))
		{
			send(tmp->fd, message, strlen(message), 0);
		}
		tmp = tmp->next;
	}
}

char *get_client_str(int fd)
{
	t_client *tmp = client;
	while (tmp)
	{
		if (tmp->fd == fd)
			return tmp->str;
		tmp = tmp->next;
	}
	return NULL;
}

void set_client_str(int fd, char *to_copy)
{
	t_client *tmp = client;
	while (tmp)
	{
		if (tmp->fd == fd)
			tmp->str = to_copy;
		tmp = tmp->next;
	}
}

void free_all()
{
	t_client *tmp = client;
	t_client *to_free = NULL;
	while (tmp)
	{
		to_free = tmp;
		tmp = tmp->next;
		free(to_free);
		to_free = NULL;
	}
}

int main(int ac, char **av)
{
	int sockfd, connfd;
	socklen_t len;
	struct sockaddr_in servaddr, cli;

	if (ac != 2)
	{
		write_msg(2, "Wrong number of arguments\n");
		exit(1);
	}
	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		write_msg(2, "Fatal error\n");
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
	{
		write_msg(2, "Fatal error\n");
		close(sockfd);
		exit(1);
	}
	if (listen(sockfd, 10) != 0)
	{
		write_msg(2, "Fatal error\n");
		exit(1);
	}
	FD_ZERO(&curr_sock);
	FD_SET(sockfd, &curr_sock);
	bzero(&msg, sizeof(msg));
	while (1)
	{
		cpy_read = cpy_write = curr_sock;
		if (select(get_max_fd(sockfd) + 1, &cpy_read, &cpy_write, NULL, NULL) < 0)
			continue;
		for (int fd = 0; fd <= get_max_fd(sockfd); fd++)
		{
			if (!FD_ISSET(fd, &cpy_read))
				continue;
			if (fd == sockfd)
			{
				len = sizeof(cli);
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				//	if (connfd < 0) {
				//		write_msg(2, "Fatal error\n");
				//		exit(1);
				//	}
				bzero(&msg, sizeof(msg));
				sprintf(msg, "server: client %d just arrived\n", add_client(connfd, sockfd));
				send_all(connfd, msg);
				FD_SET(connfd, &curr_sock);
				break;
			}
			else
			{
				bzero(&buffer, sizeof(buffer));
				int ret = recv(fd, buffer, 1000, 0);
				if (ret > 0)
				{
					printf("buffer len: %lu str: %s\n", strlen(buffer), buffer);
					strad = str_join(get_client_str(fd), buffer);
					printf("stard len: %lu\n", strlen(strad));
					while (extract_message(&strad, &bufad) == 1)
					{
						send_msg = calloc(strlen(bufad) + 42, sizeof(char));
						sprintf(send_msg, "client %d: %s", get_id(fd), bufad);
						printf("msg len: %lu\n", strlen(send_msg));
						send_all(fd, send_msg);
						free(send_msg);
						send_msg = NULL;
					}
					set_client_str(fd, strad);
				}
				else
				{
					bzero(&msg, sizeof(msg));
					sprintf(msg, "server: client %d just left\n", rm_client(fd));
					send_all(fd, msg);
					FD_CLR(fd, &curr_sock);
					close(fd);
					break;
				}
			}
		}
	}
	free(strad);
	strad = NULL;
	free_all();
	return 0;
}
