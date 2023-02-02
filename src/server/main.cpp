#include "chatserver.h"
#include "chatservice.h"
#include <signal.h>
//
void resetHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}

int main() {
    signal(SIGINT, resetHandler);
    EventLoop loop;
    InetAddress listedAddr("127.0.0.1", 8888);
    ChatServer server(&loop, listedAddr, "chat");
    server.start();
    loop.loop();
    return 0;
}