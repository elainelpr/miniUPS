#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
using namespace std;
int invoke_client(const char *hostname, const char *port);
int invoke_server(const char *port);
int Connection(int client_socket_fd, string * ip);//ip:成功与server connect的client地址结构(IP+port)的IP地址
//return 能与server进行数据通信的socket对应的fd
int getPort(int sock_fd);
