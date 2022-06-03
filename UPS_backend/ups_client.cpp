#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
//#include <error.h>
#include <fstream>
#include <mutex>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <pqxx/pqxx>
#include <thread>


#include "ups_client.hpp"

int NumOfRequest = 0;

using namespace pqxx;
using namespace std;
pqxx::connection *C;
/************************************
     Connect Database
*************************************/
void accept_db(){
    try {
    // Establish a connection to the database                                   
    // Parameters: database name, user name, user password    
    string cmd = "dbname=ups user=postgres password=passw0rd";                 
    C = new pqxx::connection(cmd);
    if (C->is_open()) {
      cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
       cout << "Can't open database" << endl;
    }
  } catch (const std::exception &e) {
    cerr << e.what() << std::endl;
  }

}

void execute(string sql) {
  //  transaction<serializable,read_write> W(*C); 
  work W(*C);
  W.exec(sql);

  W.commit();
  cout<<sql<<endl;
}
result run_query(string sql) {
  work W(*C);
  result R(W.exec(sql));
  return R;
}

void accept_world(){
  world_fd = invoke_client("vcm-24828.vm.duke.edu", "12345");
  cout << "Waiting for connection on port 12345" << endl;
}
/************************************
     Create a World
*************************************/
void initial_world(){
  //check if worldID exist,  
  //yes: UConnect: worldID,不新建trucks，isAmazon=0?
  //no: UConnect: "", truck_num, x, y; 将trucks信息加入数据库

  //UConnected return worldid and result
  // if result=connected continue!
  // else, continue sending UConnect
  UConnect ucon;
  UInitTruck trucks;
  int truck_num = 500;
  for(int i = 0; i < truck_num; ++i) {
    UInitTruck * trucks = ucon.add_trucks();
    trucks->set_id(i);
    trucks->set_x(i);
    trucks->set_y(i);
    //string sql = "INSERT INTO UPS_TRUCK(TRUCKID, TRUCKX, TRUCKY, STATUS) VALUES("+to_string(i) + "," + to_string(0) + "," + to_string(0) + "," + to_string(0) + ");";
    //execute(sql); 
  }

  ucon.set_isamazon(false);
  // send UConnect Command
    worldMutex.lock();
    {
    unique_ptr<google::protobuf::io::FileOutputStream> out(new google::protobuf::io::FileOutputStream(world_fd));
    if (sendMesgTo<UConnect>(ucon, out.get()) == false) {
        cerr<< "Error: cannot send UConnect to world." << endl;
    }
    }
    worldMutex.unlock();

    // Receive Uconnected Response
    UConnected urecv;
    worldMutex.lock();
    {
    unique_ptr<google::protobuf::io::FileInputStream> in(new google::protobuf::io::FileInputStream(world_fd));
    
    if (recvMesgFrom<UConnected>(urecv, in.get()) == false) {
        cerr<< "Error: cannot receive UConnected from world.";
        worldMutex.unlock();
        return;
    }
    }

    worldMutex.unlock();

    //check Uconnect
    string result = urecv.result();
    if(result != "connected!"){
        cerr<< "Error: UPS cannot connect to world."<<endl;
    }
    world_id = urecv.worldid();
    cout << "connected with worldID: " << world_id << endl;
}

void accept_amazon(){
  amazon_fd = invoke_client("vcm-24828.vm.duke.edu", "34567");
  cout << "Waiting for connection on port 2024" << endl;
  send(amazon_fd, &world_id, sizeof(world_id), 0);
  //server_handle_request(amazon_fd);
}

/************************************
     Handle AU_pick_truck
*************************************/

//package's status: (0)packing, (1)packed, (2)loading, (3)loaded, (4)delivering, (5)delivered

int pick_idle_truck(int pkg_id) {
  int truck_id;
   while(1){
    string sql = "SELECT truckid FROM ups_truck WHERE status = 0;";
    result res = run_query(sql);
    if(!res.empty()) {
        truck_id = res[0][0].as<int>();
        //insert truckid into package
        string sql = "UPDATE ups_package SET truckid_id = "+ to_string(truck_id) +" WHERE packageid = " + to_string(pkg_id) + "; ";
        package_mutex.lock();
        execute(sql);
        package_mutex.unlock();
        break;
    }
  }
  return truck_id;
}
void send_world_ack(vector<int> seq_nums){
  //将ack塞入UCommand, 发送过去
  UCommands ucom;
  //speed(1200);
  for(int i = 0; i < seq_nums.size(); ++i) {
    ucom.add_acks(seq_nums[i]);
  }
  worldMutex.lock();
  {
  unique_ptr<google::protobuf::io::FileOutputStream> out(new google::protobuf::io::FileOutputStream(world_fd));
  
    if (sendMesgTo<UCommands>(ucom, out.get()) == false) {
        cout << "Error: cannot send ack to world." << endl;
    }
  }
  worldMutex.unlock();
}

//string sql_sellerID = "SELECT ACCOUNTID, AMOUNT FROM OPENTB WHERE id = " + to_string(sellOrderID) + ";";

/************************************
     Handle Amazon_UPS
*************************************/

void packageAllFinished(UResponses ursp){
  vector<int> Picked_truck;
  for(int i = 0; i < ursp.completions_size(); ++i) {
    UFinished completions = ursp.completions(i);
    int seqnum = completions.seqnum();
    //checking whether seqnum = tracking number, 
    int truck_id = completions.truckid();
    Picked_truck.push_back(truck_id);
    int truck_x = completions.x();
    int truck_y = completions.y();
    string status = completions.status();
    truck_mutex.lock();
    string sql = "UPDATE ups_truck SET truckx = " + to_string(truck_x) + ", trucky = " + to_string(truck_y) + ", "+"status = 0 WHERE truckid = " + to_string(truck_id) + "; ";
    cout << "In parseUFinished function "<<" Truck status is idle"<<endl;
    execute(sql);
    truck_mutex.unlock();
  }
}

void parseUFinished(UResponses ursp){
  cout<<"**********************************"<<endl;
  cout<<"Enter parseUFinished function"<<endl;
  cout<<"**********************************"<<endl;
  vector<int> Picked_truck;
  //store the acks
  int ack_size = ursp.acks_size();
  for(int i = 0; i < ack_size; ++i) {
    int ack = ursp.acks(i);
    cout << "recv ack = " << ack << endl;
    seqnum_set.insert(ack);
    // //有seq=ack,received中加ack, seq_nums中删除ack;
    // if(seqnum_set.find(ack) != seqnum_set.end()) {
    //     cout << " remove it from seqnum_set." << endl;
    //     auto iter = seqnum_set.find(ack);
    //     //received.insert(make_pair(iter->first, iter->second));
    //     seqnum_set.erase(iter);
    // }
  }
  
  vector<int> world_seq;
  //checking the package numbcer in the response
  for(int i = 0; i < ursp.completions_size(); ++i) {
    UFinished completions = ursp.completions(i);
    int seqnum = completions.seqnum();
    //checking whether seqnum = tracking number, 
    int truck_id = completions.truckid();
    Picked_truck.push_back(truck_id);
    int truck_x = completions.x();
    int truck_y = completions.y();
    string status = completions.status();

    cout<<"From world: 1st Finished"<<endl;
    cout<<"Truck x :"<<truck_x<<endl;
    cout<<"Truck y :"<<truck_y<<endl;
    cout<<"Truck status : "<<status<<endl;
    cout<<"Pick Truck ID is "<<truck_id<<endl;
    
    string sql;
    string sql_package;
    string sql_truck;
    if(status == "IDLE"){
      
      return;
    }
    else{
      sql_truck = "UPDATE ups_truck SET truckx = " + to_string(truck_x) + ", trucky = " + to_string(truck_y) + ", status = 2 WHERE truckid = " + to_string(truck_id) + "; ";
      truck_mutex.lock();
      execute(sql_truck);
      truck_mutex.unlock();
      sql_package = "UPDATE ups_package SET status = 1 WHERE truckid_id = " + to_string(truck_id) + "; ";
      package_mutex.lock();
      execute(sql_package);
      package_mutex.unlock();
      sql_package = "UPDATE ups_package SET status = 1 WHERE truckid_id = " + to_string(truck_id) +" AND finish !=1"+ "; ";
      package_mutex.lock();
      execute(sql_package);
      package_mutex.unlock();
    }

    //insert truck info in ups_trucks, update the status of truck and package
    world_seq.push_back(seqnum);
  }

  send_world_ack(world_seq);

  //send UA_commands
  UA_commands ua_commands;

  for(int truck_id : Picked_truck) {
    string sql = "SELECT packageid FROM ups_package WHERE truckid_id = " + to_string(truck_id)+" AND STATUS =1" + ";";
    result res = run_query(sql);
    int pkg_id;
    if(!res.empty()) {
        pkg_id = res[0][0].as<int>();
    }
    UA_truck_picked *pick = ua_commands.add_pick();
    pick->set_shipid(pkg_id);
    pick->set_truckid(truck_id);
    //pick->set_seqnum(seq_num++);
    cout << "send UA_truck_picked to amazon, pkg_id=" << pkg_id << ", truck_id = " << truck_id << endl;
  }

  // send UCommand:truct connection between UPS and Amazon
  unique_ptr<google::protobuf::io::FileOutputStream> out(new google::protobuf::io::FileOutputStream(amazon_fd));
  amazonMutex.lock();
  if (sendMesgTo<UA_commands>(ua_commands, out.get()) == false) {
      cout << "Error: cannot send UCommand UA_truck_picked to Amazon." << endl;
  }
  amazonMutex.unlock();
  cout<<"__________________________________"<<endl;

}

void parseUDeliverMade(UResponses ursp){
 
  cout<<"**************************************"<<endl;
  cout<<"Enter parseUDeliverMade function"<<endl;
  cout<<"**************************************"<<endl;
  vector<int> delivered_trucks;
  vector<int> delivered_pkgs;
  // //store the acks
  //   int ack_size = ursp.acks_size();
  //   for(int i = 0; i < ack_size; ++i) {
  //     int ack = ursp.acks(i);
  //     //有seq=ack,received中加ack, seq_nums中删除ack;
  //     if(seqnum_set.find(ack) != seqnum_set.end()) {
  //         auto iter = seqnum_set.find(ack);
  //         //received.insert(make_pair(iter->first, iter->second));
  //         seqnum_set.erase(iter);
  //     }
  //   }

//store the acks
  int ack_size = ursp.acks_size();
  for(int i = 0; i < ack_size; ++i) {
    int ack = ursp.acks(i);
    cout << "recv ack = " << ack << endl;
    seqnum_set.insert(ack);
    // //有seq=ack,received中加ack, seq_nums中删除ack;
    // if(seqnum_set.find(ack) != seqnum_set.end()) {
    //     cout << " remove it from seqnum_set." << endl;
    //     auto iter = seqnum_set.find(ack);
    //     //received.insert(make_pair(iter->first, iter->second));
    //     seqnum_set.erase(iter);
    // }
  }


    vector<int> world_seq;
    //checking the package number in the response
    for(int i = 0; i < ursp.delivered_size(); ++i) {
      UDeliveryMade delivered = ursp.delivered(i);
      //UA_package_delivered upd;
      int seqnum = delivered.seqnum();
      //checking whether seqnum = tracking number, 
      int truck_id = delivered.truckid();
      delivered_trucks.push_back(truck_id);
      int pkg_id = delivered.packageid();
      delivered_pkgs.push_back(pkg_id);
      
      cout << "UDeliveryMade msg: truck_id=" << truck_id << ", pkg_id = " << pkg_id << ", seqnum = " << seqnum << endl;
      
      //upd.add_shipid(pkg_id);
      string s = "SELECT userx, usery FROM ups_package WHERE packageid = " + to_string(pkg_id) + ";";
      result res = run_query(s);
      if(!res.empty()) {
          double userx = res[0][0].as<double>();
         
          double usery = res[0][1].as<double>();
          
          //update ups_package: set status,
          //update ups_truck: truckx,trucky
          string sql_truck;// = "INSERT INTO ups_truck(truckx, trucky) VALUES (" + to_string(truck_x) + ", " + to_string(truck_y) + "); ";
          string sql_package;
          sql_truck = "UPDATE ups_truck SET truckx = " + to_string(userx) + ", trucky = " + to_string(usery) + " WHERE truckid = " + to_string(truck_id) + "; ";
          truck_mutex.lock();
          execute(sql_truck);
          truck_mutex.unlock();
          sql_package = "UPDATE ups_package SET status = 5 WHERE packageid = " + to_string(pkg_id)  + "; ";
          package_mutex.lock();
          execute(sql_package);
          package_mutex.unlock();
          string update_deliveryed = "UPDATE ups_package SET finish = 1 WHERE packageid = "+to_string(pkg_id)+";";
          package_mutex.lock();
          execute(update_deliveryed);
          package_mutex.unlock();
      }
      world_seq.push_back(seqnum);
    }
    send_world_ack(world_seq);

    //send UA_package_delivered
    UA_commands ua_commands;
    for(int truck_id : delivered_trucks) {
      string sql = "SELECT packageid FROM ups_package WHERE truckid_id = " + to_string(truck_id) + ";";
      result res = run_query(sql);
      int pkg_id;
      if(!res.empty()) {
          pkg_id = res[0][0].as<int>();
      }
      UA_package_delivered * deliver = ua_commands.add_deliver();
      deliver->add_shipid(pkg_id);
      //deliver->set_seqnum(seq_num++);
      cout<<"In parseAU_deliver_package Function for loop"<<endl;
      cout << "send UA_truck_picked to amazon, pkg_id=" << pkg_id << endl;

    }
   

    // send UA_commands 
    unique_ptr<google::protobuf::io::FileOutputStream> out(new google::protobuf::io::FileOutputStream(amazon_fd));
    amazonMutex.lock();
    if(sendMesgTo<UA_commands>(ua_commands, out.get()) == false) {
        cout<< "Error: cannot send UA_commands to world." << endl;
    }
    amazonMutex.unlock();
    cout<<"____________________________________________________"<<endl;
    return;
}


void parseAU_pick_truck(AU_commands ursp){
  cout<<"**************************************"<<endl;
  cout<<"Enter parseAU_pick_truck function"<<endl;
  cout<<"**************************************"<<endl;
    map<int,int> pkg_wh;
    //int pick_size = ursp.pick_size();
    //for(int i = 0; i < pick_size; ++i) {
      //pro cess each pick_truck msg
      AU_pick_truck pick_truck = ursp.pick(0);
      int tracking_number = pick_truck.trackingnumber();
      string user_name = pick_truck.accountname();
      int wh_ID = pick_truck.whid();
      pkg_wh.insert(make_pair(tracking_number, wh_ID));
      cout << "AU_pick_truck msg: tracking_number=" << tracking_number << ", wh_id = " << wh_ID << ", user_name = " << user_name << endl;
      
      //check the db whether we have that user
      string s = "SELECT id FROM auth_user WHERE username = '" + user_name + "';";
      result r = run_query(s);
      if(r.empty()) {
        cout << "username doesn't exist." << endl;
        return;
      }
      //check the db whether we have that user
      string sql_selectPackage = "SELECT packageid FROM ups_package WHERE packageid = " + to_string(tracking_number) + ";";
      result res = run_query(sql_selectPackage);
      if(!res.empty()) {
        cout << "ship_id has already existed." << endl;
        return;
      }
      int user_id = r[0][0].as<int>(); 
      int status = 0;
      if(!r.empty()) {
        //INSERT INTO OPENTB(ID, SYMBOL, AMOUNT, PRICE, ACCOUNTID) VALUES (" + to_string(id) + ", '" + symbol + "', " + to_string(amount) + ", " + to_string(price) + ", " + to_string(accountID) + ");";
        string sql = "INSERT INTO ups_package(packageid, warehouseid, status, finish, user_id) VALUES (" + to_string(tracking_number) + ", " + to_string(wh_ID) + ", " + to_string(status) + ", " +to_string(0)+","+ to_string(user_id)+ " );";
        package_mutex.lock();
        execute(sql);
        package_mutex.unlock();
      }

    //}
  //塞入ucommands
  UCommands ucom;
  //ucom.set_simspeed(1200);
  int pkg_id;
  int wh_id;
  int truck_id; 
  //把seq_num, UGoPickup放入map里
  auto it = pkg_wh.begin();
    //pick up idle truck
    pkg_id = it->first;
    wh_id = it->second;
    truck_id = pick_idle_truck(pkg_id); 
    UGoPickup * pickUps = ucom.add_pickups();
    pickUps->set_truckid(truck_id);
    pickUps->set_whid(wh_id);
    pickUps->set_seqnum(seq_num++);
    cout << "UGoPickup to world, truck_id = " << truck_id << ", wh_id = " << wh_id << ", seq_num = " << seq_num << endl;
    //seqnum_set.insert(seq_num-1);
    //update the status of truck in the db
    string sql = "UPDATE ups_truck SET status = 1 WHERE truckid = " + to_string(truck_id) + ";";
    truck_mutex.lock();
    execute(sql);
    truck_mutex.unlock();
  
  cout << "In parseAU_pick_truck function. send UGoPickup to world   "<<endl;
  cout << "truck_id = " << truck_id << ", wh_id = " << wh_id << ", seq_num = " << seq_num << endl;
  
  unique_ptr<google::protobuf::io::FileOutputStream> out(new google::protobuf::io::FileOutputStream(world_fd));
  int i =1;
  do{
      cout << "send UGoPickUp for the" << i++ << "th time"<< endl;
      if (sendMesgTo<UCommands>(ucom, out.get()) == false) {
        cout << "Error: cannot send UCommand to world." << endl;
      }
      else{
        cout << "send UGoPickUp!" << endl;
      }
      sleep(7);
  }while(seqnum_set.find(seq_num-1) == seqnum_set.end());

  //send_UGoPickUp(ucom);
}


void parseAU_deliver_package(AU_commands msg){
  cout<<"*************************************"<<endl;
  cout<<"Enter parseAU_deliver_package Function"<<endl;
  cout<<"*************************************"<<endl;
  UCommands ucom;
  //ucom.set_simspeed(1200);
  //int deliver_size = msg.deliver_size();
    AU_deliver_package adp = msg.deliver(0);// = msg.deliver(i);
    UGoDeliver * ugd =ucom.add_deliveries();
    //process package location
    int truck_id = adp.truckid();
    //seq_nums.insert(make_pair(seq_num, truck_id));
    //将truckid塞入 AU_deliver_package
    ugd->set_truckid(truck_id);
    ugd->set_seqnum(seq_num++);
    cout << "UGoDeliver msg: truck_id = " << truck_id << ", seq_num = " << seq_num << endl;
    int pkg_size = adp.packages_size();
    for(int i = 0; i < pkg_size; ++i) {
        UDeliveryLocation pkg = adp.packages(i);
        //将Udeliverlocation 塞入 AU_deliver_package;
        UDeliveryLocation * spkg = ugd->add_packages();
        int packageid = pkg.packageid();
        cout<<"In AU_deliver_package: pakcageid "<<packageid<<endl;
        int user_x = pkg.x();
        int user_y = pkg.y();
        cout << "In AU_deliver_package: user_x "<<user_x<<endl;
        cout << "In AU_deliver_package: user_y "<<user_y<<endl;
        spkg->set_packageid(packageid);
        spkg->set_x(user_x);
        spkg->set_y(user_y);
        cout << "UDeliveryLocation msg: packageid=" << packageid << ", user_x = " << user_x << ", user_y = " << user_y << endl;

        //update the location of package
        string sql_des = "UPDATE UPS_PACKAGE SET userx = "+to_string(user_x) + ", usery = "+to_string(user_y) + "WHERE packageid = "+to_string(packageid)+";";
        //package_mutex.lock();
        execute(sql_des);
        //package_mutex.unlock();
        cout<<"In AU_deliver_package: before sql_des, packageid is "<< packageid <<endl;
        string sql_status = "UPDATE UPS_PACKAGE SET STATUS = 3 WHERE packageid = "+to_string(packageid);
        //package_mutex.lock();
        execute(sql_status);
        //package_mutex.unlock();
    }
    //update the status of truck

    string sql = "UPDATE ups_truck SET status = 4 WHERE truckid = " + to_string(truck_id) + ";";
    truck_mutex.lock();
    execute(sql);
    truck_mutex.unlock();
    //seqnum_set.insert(seq_num-1);
    cout<<"_____________________________________"<<endl;

  unique_ptr<google::protobuf::io::FileOutputStream> out(new google::protobuf::io::FileOutputStream(world_fd));
  int i = 1;
  do{
      cout << "send UGoDeliver for the" << i++ << "th time"<< endl;
      if (sendMesgTo<UCommands>(ucom, out.get()) == false) {
        cout << "Error: cannot send UCommand to world." << endl;
      }
      else{
        cout << "send UGoDeliver!" << endl;
      }
      sleep(7);
  }while(seqnum_set.find(seq_num-1) == seqnum_set.end());

  
  //send_UGoDeliver(ucom);
  
  cout<<"________________________________________"<<endl;
}


void handleworld(UResponses ursp){
 
  if(ursp.delivered_size()!=0){
    parseUDeliverMade(ursp);
  }
  if(ursp.completions_size()!=0){
    for(int i=0; i<ursp.completions_size(); i++){
      UFinished handle = ursp.completions(i);
      if(handle.status()=="IDLE"){
        packageAllFinished(ursp);
      }
      else{
        parseUFinished(ursp);
      }
    }
  }
}

void handleamazon(AU_commands msg){
  
  if(msg.pick_size()!=0){
    parseAU_pick_truck(msg);
  }
  if(msg.deliver_size()!=0){
    parseAU_deliver_package(msg);
  }
}



void handle_thread(){
  fd_set readfds;
  int nfds = max(world_fd, amazon_fd) + 1;
  cout << "enter handle_thread" << endl;
  int i = 0;
  while(1){
    i++;
    cout<< "select : " << i << endl;
    FD_ZERO(&readfds);
    FD_SET(world_fd, &readfds);
    FD_SET(amazon_fd, &readfds);
    cout << "enter handle_thread while(1)" << endl;
    int ret = select(nfds, &readfds, NULL, NULL, NULL);
    if(ret>0){
      if(FD_ISSET(world_fd, &readfds)){
        cout << "recv world_fd:" << world_fd << endl;
        UResponses ursp;
        worldMutex.lock();
        {
        unique_ptr<google::protobuf::io::FileInputStream> in(new google::protobuf::io::FileInputStream(world_fd));
        
        if (recvMesgFrom<UResponses>(ursp, in.get()) == false) {
          cout << "Error: cannot receive UResponses from world.";
          worldMutex.unlock();
          continue;
        }
        }
        worldMutex.unlock();
        std::thread world(handleworld, ursp);
        if(world.joinable()){
          world.detach();
        }
      }
      if(FD_ISSET(amazon_fd, &readfds)){
        cout << "recv amazon:" << amazon_fd << endl;
        AU_commands msg;
       
        unique_ptr<google::protobuf::io::FileInputStream> in(new google::protobuf::io::FileInputStream(amazon_fd));
        amazonMutex.lock();
        if (recvMesgFrom<AU_commands>(msg, in.get()) == false) {
          cout<< "Error: cannot receive AU_commands from Amazon.";
          amazonMutex.unlock();
          continue;
        }
        amazonMutex.unlock();
        std::thread amazon(handleamazon,msg);
        if(amazon.joinable()){
          amazon.detach();
        }
      }
    }
  }
}


void run(){
  accept_db();
  accept_world();
  initial_world();
  accept_amazon();
  handle_thread();
}

int main(int argc, char *argv[])
{ 
  //cout << "Waiting for connection on port " << port << endl;
  //threadpool runner{20};
  //while(1){
    try {
      seq_num = 0;
      run();
      while(1){}
      
    }
    catch (const std::exception &e) {
      cerr << e.what() << std::endl;
    }
  //}
    return 0;
}