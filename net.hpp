#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

inline bool sendLine(int fd, const std::string& s) {
    std::string out=s+"\n"; size_t sent=0;
    while(sent<out.size()) { ssize_t n=send(fd,out.data()+sent,out.size()-sent,0); if(n<=0)return false; sent+=static_cast<size_t>(n); }
    return true;
}
inline bool recvLine(int fd, std::string& out) {
    out.clear(); char c;
    while(true) { ssize_t n=recv(fd,&c,1,0); if(n<=0)return false; if(c=='\n')break; if(c!='\r')out+=c; if(out.size()>65536)return false; }
    return true;
}
inline int connectTo(const std::string& host,int port) {
    int fd=socket(AF_INET,SOCK_STREAM,0); if(fd<0)return -1;
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port);
    if(inet_pton(AF_INET,host.c_str(),&a.sin_addr)!=1 || connect(fd,(sockaddr*)&a,sizeof(a))<0){close(fd);return -1;}
    return fd;
}
inline int listenOn(int port) {
    int fd=socket(AF_INET,SOCK_STREAM,0); if(fd<0)return -1;
    int yes=1; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    sockaddr_in a{}; a.sin_family=AF_INET; a.sin_addr.s_addr=INADDR_ANY; a.sin_port=htons(port);
    if(bind(fd,(sockaddr*)&a,sizeof(a))<0 || listen(fd,20)<0){close(fd);return -1;} return fd;
}
inline std::vector<std::string> split(const std::string& s,char d) {
    std::vector<std::string> v; std::stringstream ss(s); std::string x; while(std::getline(ss,x,d))v.push_back(x); return v;
}
inline std::string cleanField(std::string s) {
    for (char& c : s) {
        if (c == '|' || c == '\n' || c == '\r') c = ' ';
    }
    return s;
}
inline std::string request(const std::string& host,int port,const std::string& cmd) {
    int fd=connectTo(host,port); if(fd<0)return "ERR|Cannot connect to upstream server";
    std::string r="ERR|No response"; if(sendLine(fd,cmd))recvLine(fd,r); close(fd); return r;
}
