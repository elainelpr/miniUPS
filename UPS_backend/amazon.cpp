#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <thread>
#include "socket.hpp"
using namespace std;
int requestNUM = 0;
#define MAX_THREAD 1
//I'm amazon server.
void *handler(void *arg){
  requestNUM++;
  //const char *hostname = "";
  const char *port = "23456";
  int amazon_fd = invoke_server(port);
  string ups_ip;
  cout << "amazon_fd = " << amazon_fd << endl;
  int ups_fd = Connection(amazon_fd, &ups_ip); 
  
//   const char *Filename = (char *)arg;
//   string File(Filename);
//   string lines;
//   string s;
//   ifstream ifs(File.c_str());
//   if(ifs.fail()) {
//     cerr << "File to open XML file." << endl;
//     exit(EXIT_FAILURE);
//   }
//   while(getline(ifs, s)){
//     lines += s;
//   }
//   ifs.close();

string lines = "Good Night! I'm listening to MAMAMOO";
int length = lines.length();
  //cout << "Size of XML file is :" << length << endl; 
  send(ups_fd, &length, sizeof(length), 0);
  //cout << "Content of lines is :" << lines << endl;
  send(ups_fd, lines.c_str(),lines.length(), 0);
  char buffer[10240];
  int lengthRSP;
  recv(ups_fd, &lengthRSP, sizeof(lengthRSP), 0);
  recv(ups_fd, buffer, lengthRSP, 0);

  string rsp(buffer, buffer + lengthRSP);
  cout << requestNUM<< ": \n" << rsp <<"Don't be ridiculous"<< endl;
  close(ups_fd);
  return 0;
}

int main(int argc, char **argv) {
  try {    
    //int MAX_THREAD = stoi(argv[2]);
    int threads[MAX_THREAD];
    pthread_attr_t thread_attr[MAX_THREAD];
    pthread_t thread_ids[MAX_THREAD];
    
    for (int i = 0; i < MAX_THREAD; ++i) {
      threads[i] = pthread_create(&thread_ids[i], NULL, handler, argv[1]);
      usleep(1000);
    }
    for (int i = 0; i < MAX_THREAD; ++i) {
      pthread_join(thread_ids[i], NULL);
    }
  } catch (const std::exception &e) {
    cerr << e.what() << std::endl;
    return 1;
  }
  cout << requestNUM <<endl;
  return 0;
}
