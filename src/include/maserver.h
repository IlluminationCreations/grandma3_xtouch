#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <functional>
#include <vector>
#include <IPC.h>

class MaUDPServer {
private:
    int m_sockfd;
    struct sockaddr_in m_server_addr;

    ssize_t _sendimpl(const void *buf, size_t len);
    ssize_t _recvimpl(void *buf, size_t len);

public:
    MaUDPServer();
    ssize_t Send(char *data, uint32_t size);
    ssize_t Read(char *data, uint32_t size);
    void SendSystemButton(IPC::ButtonEvent::KeyType type, bool down);
};