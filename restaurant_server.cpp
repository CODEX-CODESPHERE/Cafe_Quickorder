#include "net.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <mutex>
#include <thread>
#include <iomanip>
#include <algorithm>
using namespace std;

struct Item { int id; string name; int price; bool available; };
struct Line { int item; int qty; };
struct Order { int id; string student; string customer; vector<Line> lines; int total=0; bool active=true; };
static vector<Item> menu={{1,"Sandwich",60,true},{2,"Veg Burger",80,true},{3,"Cold Coffee",70,true},{4,"Maggi",50,true},{5,"French Fries",65,true},{6,"Masala Dosa",90,true}};
static vector<Order> orders; static int nextId=1001; static mutex dataMutex;
static const string DB="restaurant_data.txt";
void load(){
 ifstream f(DB); string line;
 while(getline(f,line)){auto p=split(line,'|'); if(p.size()<6)continue; try{
  Order o; o.id=stoi(p[0]);o.student=p[1];o.customer=p[2];o.total=stoi(p[3]);o.active=(p[4]=="1");
  for(auto z:split(p[5],';')){auto q=split(z,':');if(q.size()==2)o.lines.push_back({stoi(q[0]),stoi(q[1])});}
  orders.push_back(o);nextId=max(nextId,o.id+1);
 }catch(...){}}
}
void save(){ofstream f(DB);for(auto&o:orders){f<<o.id<<'|'<<o.student<<'|'<<cleanField(o.customer)<<'|'<<o.total<<'|'<<(o.active?1:0)<<'|';for(size_t i=0;i<o.lines.size();++i){if(i)f<<';';f<<o.lines[i].item<<':'<<o.lines[i].qty;}f<<'\n';}}
string menuText(){ostringstream s;s<<"OK|ID  ITEM                 PRICE STATUS | ";for(auto&i:menu)s<<i.id<<"   "<<left<<setw(21)<<i.name<<" Rs."<<setw(5)<<i.price<<(i.available?"Available":"Unavailable")<<" ; ";return s.str();}
string handle(const string& cmd){
 auto p=split(cmd,'|'); if(p.empty())return "ERR|Empty command";
 lock_guard<mutex> lock(dataMutex);
 if(p[0]=="PING")return "OK|PONG";
 if(p[0]=="MENU")return menuText();
 if(p[0]=="PLACE" && p.size()==4){
  string sid=p[1], customer=p[2]; Order o; o.id=nextId++;o.student=sid;o.customer=customer;
  for(auto entry:split(p[3],',')){auto q=split(entry,':');if(q.size()!=2)return "ERR|Invalid item list";int id,qty;try{id=stoi(q[0]);qty=stoi(q[1]);}catch(...){return "ERR|Invalid item or quantity";}
   auto it=find_if(menu.begin(),menu.end(),[&](const Item&x){return x.id==id;});
   if(it==menu.end()||!it->available||qty<1||qty>30)return "ERR|Invalid or unavailable item/quantity";
   auto li=find_if(o.lines.begin(),o.lines.end(),[&](const Line&x){return x.item==id;});
   if(li==o.lines.end())o.lines.push_back({id,qty});else li->qty+=qty;
   o.total+=it->price*qty;
  }
  if(o.lines.empty())return "ERR|Order cannot be empty";
  orders.push_back(o);save();return "OK|Order placed. Order ID: "+to_string(o.id)+" Total: Rs."+to_string(o.total);
 }
 if(p[0]=="MYORDERS" && p.size()==2){ostringstream s;s<<"OK|Your orders: ";bool found=false;for(auto&o:orders)if(o.student==p[1]){found=true;s<<"ID "<<o.id<<" | "<<o.customer<<" | Rs."<<o.total<<" | "<<(o.active?"ACTIVE":"CANCELLED")<<" ; ";}if(!found)s<<"No orders found";return s.str();}
 if(p[0]=="CANCEL" && p.size()==3){int id;try{id=stoi(p[2]);}catch(...){return "ERR|Invalid order ID";}for(auto&o:orders)if(o.id==id&&o.student==p[1]){if(!o.active)return "ERR|Order already cancelled";o.active=false;save();return "OK|Order cancelled";}return "ERR|Order not found";}
 if(p[0]=="ALLORDERS"){ostringstream s;s<<"OK|ALL ORDERS | ";for(auto&o:orders)s<<o.id<<" | Student "<<o.student<<" | "<<o.customer<<" | Rs."<<o.total<<" | "<<(o.active?"ACTIVE":"CANCELLED")<<" ; ";return s.str();}
 if(p[0]=="REPORT"){int active=0,cancel=0,revenue=0;for(auto&o:orders)if(o.active){active++;revenue+=o.total;}else cancel++;ostringstream s;s<<"OK|SALES REPORT | Orders: "<<orders.size()<<" | Active: "<<active<<" | Cancelled: "<<cancel<<" | Revenue: Rs."<<revenue;return s.str();}
 if(p[0]=="AVAIL"&&p.size()==3){int id;try{id=stoi(p[1]);}catch(...){return "ERR|Invalid ID";}for(auto&i:menu)if(i.id==id){i.available=(p[2]=="1");return "OK|Availability updated";}return "ERR|Item not found";}
 return "ERR|Unknown command or invalid arguments";
}
void client(int fd){string cmd;while(recvLine(fd,cmd)){string out=handle(cmd);if(!sendLine(fd,out))break;}close(fd);}
int main(int argc,char**argv){int port=argc>1?stoi(argv[1]):5001;load();int server=listenOn(port);if(server<0){cerr<<"Cannot listen on port "<<port<<". Is it already in use?\n";return 1;}cout<<"Restaurant server listening on port "<<port<<"\n";cout<<"Data file: "<<DB<<"\nPress Ctrl+C to stop.\n";while(true){int fd=accept(server,nullptr,nullptr);if(fd<0)continue;thread(client,fd).detach();}}
