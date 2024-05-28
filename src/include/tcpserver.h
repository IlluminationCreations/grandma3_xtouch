#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <alive.h>

constexpr int BUFSIZE = 1058;

class TCPServer : Alive {
private:
    struct {
        int sockfd;
        socklen_t clientlen;
        struct sockaddr_in clientaddr;
    } m_socket;

    void *buffer;

private:
    void Bind(unsigned short port);

public:
    TCPServer(unsigned short port);
    bool Alive();
};