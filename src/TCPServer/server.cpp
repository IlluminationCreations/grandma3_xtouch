
#include <tcpserver.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TCPServer::TCPServer(unsigned short port) {
    buffer = malloc(BUFSIZE);
    Bind(port);

}

void TCPServer::Bind(unsigned short port) {
    try {
        struct sockaddr_in serveraddr;
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serveraddr.sin_port = htons(port);

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
        SetLifeState(true);
    }
    catch (...) {
        SetLifeState(false);
    }

}

bool TCPServer::Alive() {
    return IsAlive();
}