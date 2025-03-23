#ifndef SOCKET_H
#define SOCKET_H

#ifdef _WIN32
    #include <winsock2.h>
    typedef SOCKET socket_t;
#else
    typedef int socket_t;
    #define INVALID_SOCKET (-1)
#endif

void initializeNetworking(void);
void cleanupNetworking(void);
void setNonBlocking(socket_t sock);
int pollSocket(socket_t client_sock);
void sendCounter(socket_t client_sock, unsigned int counter);
void closeSocket(socket_t sock);

#endif // SOCKET_H
