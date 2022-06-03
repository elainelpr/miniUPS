#include "threadpool.hpp"
#include "send_recv.hpp"
#include "socket.hpp"
#include "protobuf/world_ups.pb.h"
#include <fstream>
#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <map>
#include <ctime>
#include <vector>
#include <pqxx/pqxx>
#include <set>
using namespace pqxx;
using namespace std;
typedef google::protobuf::io::FileOutputStream ups_out;
typedef google::protobuf::io::FileInputStream ups_in;
int world_fd;
std::mutex worldMutex;
int amazon_fd;
std::mutex amazonMutex;
long long world_id;
int seq_num;
std::mutex truck_mutex;
std::mutex package_mutex;
std::mutex user_mutex;
set<int> seqnum_set;

class truck{
public:
    int id;
    int x, y;
    int pkg_id;
    truck(int id, int x, int y){}
    ~truck(){}
};

    
void server_handle_request(int client_connection_fd);
void accept_world();//
void accept_db();
void initial_world();
void accept_amazon();
void send_world_ack(vector<int> seqnum);
int pick_idle_truck(int pkg_id);
void parseUDeliverMade(UResponses ursp);
void send_UGoPickUp(UCommands ucom);
void parseAU_pick_truck(AU_commands ursp);
void send_UGoDeliver(UCommands ucom);
void parseAU_deliver_package(AU_commands ursp);
void disconnect_world();
void run();