/*
This is a simple program that demonstrates how to use non-blocking I/O 
with on both the keyboard and network socket for Windows, Linux and
macOS. It also uses ANSI escape codes to draw a simple box drawin using
UTF-8 box characters.

The program displays a counter in a box on the terminal screen and 
increments the counter every 100 milliseconds. The counter can be
reset by pressing the 'r' key and the counter value can be sent to a
connected client by pressing the 'w' key. The program also listens for
incoming data from the client and sends the counter value back to the
client when a carriage return ('\r') is received.

Compilation instructions:

On Windows (using MinGW):
    gcc main.c serial.c socket.c -lws2_32 -o main.exe

On macOS/Linux:
    gcc main.c serial.c socket.c -o main

This code is released as public domain. Use it as you wish.

March 2025, Mats Engstrom - @matseng  - github.com/mengstr
*/

#define PORT 7777

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
    #include <conio.h>
    #pragma comment(lib, "ws2_32.lib")
    #define Printf(fmt, ...) wprintf(L"" fmt, ##__VA_ARGS__)
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>

    #define Printf(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define Sleep(ms) usleep((ms)*1000)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include "serial.h"
#include "socket.h"

// ============================ Main Program ================================
int main(void) {
    initializeNetworking();
    initializeTerminal();

    // Use the system locale (UTF-8).
    setlocale(LC_ALL, "");

    // ================== Socket Setup =====================
    socket_t listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(listen_sock, 1) < 0) {
        perror("listen");
        exit(1);
    }
    // Set listening socket to non-blocking mode.
    setNonBlocking(listen_sock);
    socket_t client_sock = INVALID_SOCKET;

    // ================== Box Drawing =======================
    int start_row = 10;   // row where the box starts (1-indexed)
    int start_col = 30;   // column where the box starts (1-indexed)
    int width = 20;       // total width of the box
    int height = 7;       // total height of the box

    // Clear screen and draw the box border.
    Printf("\033[2J\033[H");   // Clear screen & home cursor.
    Printf("\033[31m");        // Set text color to red.
    // Top border.
    Printf("\033[%d;%dH┌", start_row, start_col);
    for (int i = 0; i < width - 2; i++) {
        Printf("─");
    }
    Printf("┐");
    // Sides.
    for (int row = 1; row < height - 1; row++) {
        Printf("\033[%d;%dH│", start_row + row, start_col);
        for (int col = 0; col < width - 2; col++) {
            Printf(" ");
        }
        Printf("│");
    }
    // Bottom border.
    Printf("\033[%d;%dH└", start_row + height - 1, start_col);
    for (int i = 0; i < width - 2; i++) {
        Printf("─");
    }
    Printf("┘");
    // Reset text attributes.
    Printf("\033[0m");

    // Calculate position to display the counter (roughly centered in the box).
    int counter_row = start_row + height / 2;
    int counter_col = start_col + (width / 2) - 2;  // Adjust for centering

    unsigned int counter = 0;
    int key;
    int sockKey;

    // ================== Main Loop ==========================
    while (1) {
        // Accept a new connection if no client is connected.
        if (client_sock == INVALID_SOCKET) {
            client_sock = accept(listen_sock, NULL, NULL);
            if (client_sock != INVALID_SOCKET) {
                setNonBlocking(client_sock);
            }
        }

        // Poll keyboard.
        key = pollKey();
        if (key != -1) {
            if (key == 'q' || key == 'Q') {
                break;  // exit the loop.
            } else if (key == 'r' || key == 'R') {
                counter = 0;  // reset counter.
            } else if (key == 'w') {  // w from keyboard.
                sendCounter(client_sock, counter);
            }
        }

        // Poll socket if connected.
        if (client_sock != INVALID_SOCKET) {
            sockKey = pollSocket(client_sock);
            if (sockKey == '\r') {  // CR received from socket.
                sendCounter(client_sock, counter);
            } else if (sockKey == -2) { // Client disconnected.
                // Close the client socket and allow a new connection.
                closeSocket(client_sock);
                client_sock = INVALID_SOCKET;
            }
        }

        // Update the counter display inside the box.
        Printf("\033[%d;%dH", counter_row, counter_col);
        Printf("%4u", counter);
        fflush(stdout);

        Sleep(100);  // wait 100 milliseconds.
        counter++;
    }

    // Cleanup: close sockets and restore terminal.
    if (client_sock != INVALID_SOCKET) {
        closeSocket(client_sock);
        closeSocket(listen_sock);
        cleanupTerminal();
        cleanupNetworking();
    }

    // Move the cursor below the box before exiting.
    Printf("\033[%d;0H", start_row + height + 2);
    return 0;
}