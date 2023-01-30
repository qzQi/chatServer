#include "chatserver.h"

int main(){
    EventLoop loop;
    InetAddress listedAddr("127.0.0.1",8888);
    ChatServer server(&loop,listedAddr,"chat");
    server.start();
    loop.loop();
    return 0;
}