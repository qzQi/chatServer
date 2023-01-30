#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <string>
#include<functional>//for bind

#include "json.hpp"

using namespace std;
using namespace placeholders;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

class Server {
  public:
    Server(EventLoop *loop, const InetAddress &listenAddr, const string &name);

    void start(){
        _server.start();
    }

    void onConnection(const TcpConnectionPtr &conn);

    void onMessage(const TcpConnectionPtr &conn, Buffer *buff, Timestamp t);

  private:
    TcpServer _server;
    EventLoop *_loop;
};

Server::Server(
    EventLoop *loop, const InetAddress &listenAddr, const string &name)
    : _loop(loop), _server(loop,listenAddr,name) {

        _server.setConnectionCallback(std::bind(&Server::onConnection,this,_1));

        _server.setMessageCallback(std::bind(&Server::onMessage,this,_1,_2,_3));

        _server.setThreadNum(4);
}

void Server::onConnection(const TcpConnectionPtr& conn){
    if(!conn->connected()){
        LOG_INFO<<"conn: "<<conn->name()<<" closed";
    }
}

void Server::onMessage(const TcpConnectionPtr& conn,Buffer* buff,Timestamp t){
    string msg=buff->retrieveAllAsString();
    json js=json::parse(msg);
    LOG_INFO<<"msg from "<<js["name"].get<string>()<<" at"<<t.toFormattedString();
}

int main(){
    EventLoop loop;
    InetAddress listedAddr("127.0.0.1",8888);
    Server server(&loop,listedAddr,"Server");

    server.start();
    loop.loop();
}