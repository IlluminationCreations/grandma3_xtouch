#include <iostream>
#include <cstring>

#include <unistd.h>
#include <assert.h>
#include <maserver.h>
#include <XController.h>
#include <IPC.h>

#define SERVER_PORT 9000
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 4096

MaUDPServer::MaUDPServer() {
    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockfd < 0) {
        std::cerr << "ERROR opening socket\n";
    }
    memset(&m_server_addr, 0, sizeof(m_server_addr));
    m_server_addr.sin_family = AF_INET;
    m_server_addr.sin_port = htons(SERVER_PORT);
    m_server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    setsockopt (m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
}

ssize_t MaUDPServer::_sendimpl(const void *buf, size_t len) {
    return sendto(m_sockfd, buf, len, 0, (const struct sockaddr *)&m_server_addr, sizeof(m_server_addr));
}

ssize_t MaUDPServer::Send(char *data, uint32_t size) {
    return _sendimpl(data, size);
}
ssize_t MaUDPServer::_recvimpl(void *buf, size_t len) {
    socklen_t l = sizeof(m_server_addr);
    return recvfrom(m_sockfd, buf, len, 0, (struct sockaddr *)&m_server_addr, &l);
}

ssize_t MaUDPServer::Read(char *data, uint32_t size) {
    return _recvimpl(data, size);
}

void MaUDPServer::SendSystemButton(IPC::ButtonEvent::KeyType type, bool down) {
    IPC::IPCHeader header;
    header.type = IPC::PacketType::PRESS_MA_SYSTEM_KEY;
    header.seq = 0;
    IPC::ButtonEvent::SystemKeyDown event;
    event.key = type;
    event.down = down;
    char *data = (char*)malloc(sizeof(IPC::IPCHeader) + sizeof(IPC::ButtonEvent::SystemKeyDown));
    memcpy(data, &header, sizeof(IPC::IPCHeader));
    memcpy(data + sizeof(IPC::IPCHeader), &event, sizeof(IPC::ButtonEvent::SystemKeyDown));
    Send(data, sizeof(IPC::IPCHeader) + sizeof(IPC::ButtonEvent::SystemKeyDown));
    free(data);
}

// uint32_t StartCommunication() {
//     char *data = (char*)malloc(BUFFER_SIZE);
//     memcpy(data, &header, sizeof(IPC::IPCHeader));
//     memcpy(data + sizeof(IPC::IPCHeader), &request, sizeof(IPC::PlaybackRefresh::Request));
//     auto total_size = sizeof(IPC::IPCHeader) + sizeof(IPC::PlaybackRefresh::Request);
//     // Send message to server
//     ssize_t send_result = sendto(sockfd, data, total_size, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
//     if (send_result < 0) {
//         std::cerr << "Send message failed" << std::endl;
//         close(sockfd);
//         return -1;
//     }

//     std::cout << "Message sent to server: " << std::endl;

//     // Receive response from server
//     socklen_t len = sizeof(server_addr);
//     ssize_t recv_result = recvfrom(sockfd, data, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &len);
//     printf("recv_result: %zd\n", recv_result);
//     assert(recv_result == sizeof(IPC::IPCHeader) + sizeof(IPC::PlaybackRefresh::ChannelMetadata));
//     IPC::IPCHeader resp_header;
//     IPC::PlaybackRefresh::ChannelMetadata resp_metadata;
//     memcpy(&resp_header, data, sizeof(IPC::IPCHeader));
//     memcpy(&resp_metadata, data + sizeof(IPC::IPCHeader), sizeof(IPC::PlaybackRefresh::ChannelMetadata));
//     assert(resp_header.type == IPC::PacketType::RESP_ENCODERS);
//     assert(resp_header.seq == 1337);


//      printf("\n");
//      printf("\n");
//      printf("\n");
//      printf("\n");
//      printf("\n");
//      printf("\n");
//      printf("\n");

//     for(int i = 0; i < 8; i++) {
//         if (resp_metadata.channelActive[i]) {
//             printf("Channel %d: Active\n", i + 1);
//         } else {
//             printf("Channel %d: Inactive\n", i + 1);
//             continue;
//         }

//         recv_result = recvfrom(sockfd, data, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &len);
//         IPC::PlaybackRefresh::Data *res = (IPC::PlaybackRefresh::Data*)data;
//         assert(recv_result == sizeof(IPC::PlaybackRefresh::Data));

//         printf("page: %d\n", res->page);
//         printf("channel: %d\n", res->channel);
//         for(int i = 0; i < 3; i++) {
//             char key_name[9];
//             memcpy(key_name, res->Encoders[i].key_name, 8);
//             key_name[8] = '\0';
//             printf("Encoders[%d].isActive: %d\n", i, res->Encoders[i].isActive);
//             printf("Encoders[%d].key_name: %s\n", i, key_name);
//             printf("Encoders[%d].value: %f\n", i, res->Encoders[i].value);
//         }
//         for(int i = 0; i < 4; i++) {
//             printf("keysActive[%d]: %d\n", i, res->keysActive[i]);
//         }
//         printf("\n");

//     }
//     printf("Grand Master: %f\n", resp_metadata.master);
//     printf("\n");

//     close(sockfd);
//     return 0;
// }
