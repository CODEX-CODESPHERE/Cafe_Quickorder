#include "net.hpp"
#include <iostream>
#include <string>
using namespace std;
int main(int argc,char**argv){string host=argc>1?argv[1]:"127.0.0.1";int port=argc>2?stoi(argv[2]):5001;cout<<"=== RESTAURANT ADMIN TERMINAL ===\n";while(true){cout<<"\n1 View menu\n2 View all orders\n3 Sales report\n4 Update item availability\n0 Exit\nChoice: ";string c;getline(cin,c);string cmd;
 if(c=="1")cmd="MENU";else if(c=="2")cmd="ALLORDERS";else if(c=="3")cmd="REPORT";else if(c=="4"){string id,v;cout<<"Item ID: ";getline(cin,id);cout<<"Available? (1=yes, 0=no): ";getline(cin,v);cmd="AVAIL|"+id+"|"+v;}else if(c=="0")break;else{cout<<"Invalid choice\n";continue;}
 cout<<request(host,port,cmd)<<"\n";
 }return 0;}
