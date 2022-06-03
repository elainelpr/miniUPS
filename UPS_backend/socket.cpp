#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include "socket.hpp"
using namespace std;
int invoke_client(const char *hostname, const char *port) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  //hostname and port is input
  memset(&host_info, 0, sizeof(host_info));//host_info变量重置为0
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if
  
  // cout << "Connecting to " << hostname << " on port " << port << "..." << endl;
  
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  freeaddrinfo(host_info_list);
  return socket_fd;
}

int invoke_server(const char *port) {
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  memset(&host_info, 0, sizeof(host_info));
//memset(address of element that requires initialization, initialized value, size of element)
  host_info.ai_family   = AF_UNSPEC;//unspecified
  host_info.ai_socktype = SOCK_STREAM;//
  host_info.ai_flags    = AI_PASSIVE;//socket地址用于监听绑定



  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  //fail to get address infomation
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  /*
   int socket(int domain, int type, int protocol);
   domain:选用IP地址协议：AF_INET, AF_INET6, AF_UNIX
   type:数据传输协议： SOCK_STREAM/SOCK_DGRAM
   protocol:协议的代表协议 0
   return : 成功：socket 所对应的fd; 失败：-1 errno
   */
  //分配失败
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    //int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
  /*
   level所在的协议层：SOL_SOCKET:通用套接字选项
   optname:optname：需要访问的选项名
   optval：对于getsockopt()，指向包含新选项值的缓冲。
   oplen:现选项的长度
   */
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  /*
   int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
   给socket绑定一个(IP+port_num)
   sockfd: socket()返回值
   addr: 传入参数(struct sockaddr *)&addr
   addrlen: sizeof(addr) 地址结构的大小
   return：成功0；失败-1
 */
    //bind不成功
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    //cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }

  status = listen(socket_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl; 
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  freeaddrinfo(host_info_list);
  return socket_fd;
  
}
//ip:成功与server connect的client地址结构
//return:能与server进行数据通信的socket对应的fd
int Connection(int client_socket_fd, string * ip) {
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connection_fd;
  client_connection_fd = accept(client_socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    return -1;
  }
  struct sockaddr_in * addr = (struct sockaddr_in *)&socket_addr;
  *ip = inet_ntoa(addr->sin_addr);
  //The inet_ntoa function converts an (Ipv4) Internet network address into a string in Internet standard dotted-decimal format.
  return client_connection_fd;

}


int getPort(int socket_fd) {
  struct sockaddr_in socket_addr;
  socklen_t socket_addrlen = sizeof(socket_addr);
  if(getsockname(socket_fd, (struct sockaddr *)&socket_addr, &socket_addrlen)) {
    cerr << "Error: cannot accept connection on socket" << endl;
    return -1;
  }
  return ntohs(socket_addr.sin_port);
  /* ntohs:将一个16位数由网络字节顺序转换为主机字节顺序。*/
}
