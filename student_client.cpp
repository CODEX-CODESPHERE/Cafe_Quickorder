#include "net.hpp"
#include <iostream>
#include <string>
using namespace std;
int main(int argc,char**argv){string host=argc>1?argv[1]:"127.0.0.1";int port=argc>2?stoi(argv[2]):5002;int fd=connectTo(host,port);if(fd<0){cerr<<"Cannot connect to student server "<<host<<":"<<port<<"\n";return 1;}
 cout<<"=== CAFE QUICKORDER / STUDENT TERMINAL ===\n1. Register\n2. Login\nChoice: ";string c;getline(cin,c);
 if(c=="1"){string id,name,pw;cout<<"Student ID: ";getline(cin,id);cout<<"Full name: ";getline(cin,name);cout<<"Password: ";getline(cin,pw);sendLine(fd,"REGISTER|"+cleanField(id)+"|"+cleanField(name)+"|"+cleanField(pw));string r;recvLine(fd,r);cout<<r<<"\n";if(r.rfind("OK|",0)!=0){close(fd);return 0;}cout<<"Now login.\n";}
 string id,pw;cout<<"Student ID: ";getline(cin,id);cout<<"Password: ";getline(cin,pw);sendLine(fd,"LOGIN|"+cleanField(id)+"|"+cleanField(pw));string r;if(!recvLine(fd,r)){cerr<<"Server disconnected\n";close(fd);return 1;}cout<<r<<"\n";if(r.rfind("OK|",0)!=0){close(fd);return 1;}
 while(true){cout<<"\n--- STUDENT MENU ---\n1 View restaurant menu\n2 Place order\n3 My orders\n4 Cancel order\n0 Logout\nChoice: ";getline(cin,c);string cmd;
  if(c=="1")cmd="MENU";
  else if(c=="2"){cout<<"Customer / pickup name: ";string n;getline(cin,n);cout<<"Enter itemId:quantity pairs separated by commas (example 1:2,3:1): ";string items;getline(cin,items);cmd="PLACE|"+cleanField(n)+"|"+items;}
  else if(c=="3")cmd="MYORDERS";
  else if(c=="4"){cout<<"Order ID: ";string oid;getline(cin,oid);cmd="CANCEL|"+oid;}
  else if(c=="0")break;else{cout<<"Invalid choice\n";continue;}
  if(!sendLine(fd,cmd)||!recvLine(fd,r)){cout<<"Connection lost\n";break;}cout<<r<<"\n";
 }
 close(fd);cout<<"Logged out.\n";
}
