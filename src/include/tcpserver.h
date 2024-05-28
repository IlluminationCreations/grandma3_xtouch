#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <alive.h>
#include <thread>
#include <functional>

constexpr int BUFSIZE = 1058;
using PacketCallback = std::function<void(unsigned char*, uint64_t)>;

class TCPServer : Alive {
private:
    struct {
        int sockfd;
        socklen_t clientlen;
        struct sockaddr_in clientaddr;
    } m_socket;

    unsigned char *m_buffer;
    PacketCallback m_cb;
    unsigned short m_port;
    std::thread m_recv_thread;

private:
    void Bind();
    void Start();
    void Read();

public:
    TCPServer(unsigned short port, PacketCallback);
    bool Alive();
};