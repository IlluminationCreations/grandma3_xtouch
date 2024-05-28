
#include <tcpserver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TCPServer::TCPServer(unsigned short port, PacketCallback cb) : m_cb(cb), m_port(port) {
    m_buffer =  (unsigned char *)malloc(BUFSIZE);
    memset(&m_socket, 0, sizeof(m_socket));
    Start();
}

void TCPServer::Start() {
    try 
    {
        Bind();
    }
    catch (...)
    {
        SetDead();
    }
}

void TCPServer::Send(unsigned char *buffer, unsigned int len) {
    sendto(m_socket.sockfd, buffer, len, 0, (struct sockaddr *) &(m_socket.clientaddr), (m_socket.clientlen));
}

void TCPServer::Bind() {
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(m_port);

    // ------------------------------------------------------------------------------------
    // Perform socket related initilisations
    m_socket.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket.sockfd < 0) {
        printf("ERROR opening socket\n");
    }
    int optval = 1;
    setsockopt(m_socket.sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
    if (bind(m_socket.sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        printf("ERROR on binding\n");
    }
    m_socket.clientlen = sizeof(m_socket.clientaddr);
    
    m_recv_thread = std::thread(&TCPServer::Read, this);

}

bool TCPServer::Alive() {
    return IsAlive();
}

void TCPServer::Read() {
    printf("Reader thread started\n");
    while(Alive()) {
        size_t recvlen = recvfrom(m_socket.sockfd, m_buffer, BUFSIZE, 0, (struct sockaddr *) &(m_socket.clientaddr), &(m_socket.clientlen));
        if (recvlen < 0) {
            printf("ERROR in recvfrom\n");
            SetDead();
            return;
        }
        m_cb(m_buffer, recvlen);
    }
    printf("Reader thread dead\n");
    SetDead(); // Reading thread has died
}