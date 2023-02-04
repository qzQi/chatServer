#include "chatserver.h"
#include "chatservice.h"
#include <signal.h>
#include <iostream>
using namespace std;
//
void resetHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    signal(SIGINT, resetHandler);
    EventLoop loop;
    InetAddress listedAddr(ip, port);
    ChatServer server(&loop, listedAddr, "chat");
    server.start();
    loop.loop();
    return 0;
}