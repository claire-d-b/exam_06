#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

char msg[2048], char buf[2048];
fd_set cpy_read, cpy_write, curr_sock;

typedef struct s_cli
{
    int fd;
    int id;
    struct s_cli next;
}               t_cli;

void write_msg(int fd, char *msg)
{
    write(fd, msg, strlen(msg));
}

int add_client(int fd, t_cli *client)
{
    t_cli *new = malloc(sizeof(t_cli));
    if (client->next)
    {
        while (client->next)
            client = client->next;
        client->next = new;
        client->next->id = client->id + 1;
        client->next->fd = fd;
        client->next->next = NULL;
    }
    else
    {
        client = new;
        client->id = 0;
        client->fd = fd;
        client->next = NULL;
    }
    return client->id;
}

int rm_client(int fd, t_cli *client)
{
    t_cli *tmp = client;
    while (client->next)
    {
        if (client->fd == fd)
        {
            tmp = client;
            client = client->next;
            free(tmp);
            tmp = NULL;
            return client->id;
        }
        client = client->next;
    }
    return client->id;
}

int get_id(int fd, t_cli *client)
{
    while (client->next)
    {
        if (client->fd == fd)
        {
            return client->id;
        }
        client = client->next;
    }
    return client->id;
}

int get_max_fd(t_cli *client)
{
    int max = client->fd;
    while (client->next)
    {
        if (client->fd > max)
            max = client->fd;
        client = client->next;
    }
    return max;
}

void send_all(t_cli *client, int fd, char *msg)
{
    if ((send(fd, msg, strlen(msg), 0)) == -1)
    {
        write_msg(2, "Fatal error\n");
        exit(1);
    }
}

int extract_message(char **buf, char **msg)
{
  char  *newbuf;
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
  char  *newbuf;
  int   len;

  if (buf == 0)
    len = 0;
  else
    len = strlen(buf);
  newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
  if (newbuf == 0)
    return (0);
  newbuf[0] = 0;
  if (buf != 0)
    strcat(newbuf, buf);
  free(buf);
  strcat(newbuf, add);
  return (newbuf);
}


int main() {
  t_cli client;
  int sockfd, connfd, len;
  struct sockaddr_in servaddr, cli; 

  // socket create and verification 
  sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  if (sockfd == -1) { 
    printf("socket creation failed...\n"); 
    exit(0); 
  } 
  else
    printf("Socket successfully created..\n"); 
  bzero(&servaddr, sizeof(servaddr)); 

  // assign IP, PORT 
  servaddr.sin_family = AF_INET; 
  servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
  servaddr.sin_port = htons(8081); 
  
  // Binding newly created socket to given IP and verification 
  if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
    printf("socket bind failed...\n"); 
    exit(0); 
  } 
  else
    printf("Socket successfully binded..\n");
  if (listen(sockfd, 10) != 0) {
    printf("cannot listen\n"); 
    exit(0); 
  }
  len = sizeof(cli);
  connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
  if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n");
}