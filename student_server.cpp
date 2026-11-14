#include "net.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <mutex>
#include <thread>
using namespace std;
struct Student{string name;size_t passwordHash;};
static unordered_map<string,Student> students;static mutex studentMutex;
static const string DB="students_data.txt";
static string restaurantHost="127.0.0.1";static int restaurantPort=5001;
void load(){ifstream f(DB);string id,name,hash;while(getline(f,id,'|')&&getline(f,name,'|')&&getline(f,hash)){try{students[id]={name,stoull(hash)};}catch(...){}}}
void save(){ofstream f(DB);for(auto&x:students)f<<cleanField(x.first)<<'|'<<cleanField(x.second.name)<<'|'<<x.second.passwordHash<<'\n';}
string forward(const string&cmd){return request(restaurantHost,restaurantPort,cmd);}
void client(int fd){
 string cmd,logged="";while(recvLine(fd,cmd)){auto p=split(cmd,'|');string out="ERR|Invalid command";
  if(p.size()==4&&p[0]=="REGISTER"){
   lock_guard<mutex>l(studentMutex);if(p[1].empty()||p[2].empty()||p[3].empty())out="ERR|All fields are required";
   else if(students.count(p[1]))out="ERR|Student ID already registered";
   else{students[p[1]]={p[2],hash<string>{}(p[3])};save();out="OK|Registration successful. Please log in";}
  }else if(p.size()==3&&p[0]=="LOGIN"){
   lock_guard<mutex>l(studentMutex);auto it=students.find(p[1]);if(it!=students.end()&&it->second.passwordHash==hash<string>{}(p[2])){logged=p[1];out="OK|Welcome "+it->second.name;}else out="ERR|Invalid student ID or password";
  }else if(p[0]=="PING")out="OK|PONG";
  else if(logged.empty())out="ERR|Please log in first";
  else if(p[0]=="MENU"&&p.size()==1)out=forward("MENU");
  else if(p[0]=="PLACE"&&p.size()==3){string name=cleanField(p[1]);out=forward("PLACE|"+logged+"|"+name+"|"+p[2]);}
  else if(p[0]=="MYORDERS"&&p.size()==1)out=forward("MYORDERS|"+logged);
  else if(p.size()==2&&p[0]=="CANCEL")out=forward("CANCEL|"+logged+"|"+p[1]);
  if(!sendLine(fd,out))break;
 }close(fd);
}
int main(int argc,char**argv){int port=argc>1?stoi(argv[1]):5002;if(argc>2)restaurantHost=argv[2];if(argc>3)restaurantPort=stoi(argv[3]);load();int server=listenOn(port);if(server<0){cerr<<"Cannot listen on port "<<port<<"\n";return 1;}cout<<"Student server listening on port "<<port<<"\nConnecting to restaurant server at "<<restaurantHost<<":"<<restaurantPort<<"\n";while(true){int fd=accept(server,nullptr,nullptr);if(fd>=0)thread(client,fd).detach();}}
