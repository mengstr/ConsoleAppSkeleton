#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif

#ifdef _WIN32
void initWinsock(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }
}

void cleanupWinsock(void) {
    WSACleanup();
}
#else
void initWinsock(void) {}
void cleanupWinsock(void) {}
#endif

void initializeNetworking(void) {
    initWinsock();  // Does nothing on non-Windows platforms
}

void cleanupNetworking(void) {
    cleanupWinsock();  // Does nothing on non-Windows platforms
}

void setNonBlocking(socket_t sock) {
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

void closeSocket(socket_t sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

int pollSocket(socket_t client_sock) {
    if (client_sock == INVALID_SOCKET)
        return -1;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(client_sock, &set);
    struct timeval timeout = { 0, 0 };
    
    int rv = select((int)client_sock + 1, &set, NULL, NULL, &timeout);
    if (rv > 0 && FD_ISSET(client_sock, &set)) {
        char buf[16];
        int ret = recv(client_sock, buf, sizeof(buf), 0);
        if (ret > 0) {
            for (int i = 0; i < ret; i++) {
                if (buf[i] == '\r')
                    return '\r';
            }
        } else if (ret == 0) {
            return -2;
        }
    }
    return -1;
}

void sendCounter(socket_t client_sock, unsigned int counter) {
    if (client_sock == INVALID_SOCKET)
        return;
    char outbuf[64];
    snprintf(outbuf, sizeof(outbuf), "%u\r\n", counter);
#ifdef _WIN32
    send(client_sock, outbuf, (int)strlen(outbuf), 0);
#else
    send(client_sock, outbuf, strlen(outbuf), 0);
#endif
}
