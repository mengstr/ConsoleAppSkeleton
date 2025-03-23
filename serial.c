#include "serial.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/select.h>
    #include <fcntl.h>
#endif

#ifdef _WIN32
int pollKey(void) {
    if (_kbhit())
        return _getch();
    return -1;
}

void disableRawMode(void) {
    // No special handling needed on Windows.
}

void enableRawMode(void) {
    // No special handling needed on Windows.
}

void initializeTerminal(void) {
    _setmode(_fileno(stdout), _O_U8TEXT);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) exit(1);
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) exit(1);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!SetConsoleMode(hOut, dwMode)) exit(1);
}

void cleanupTerminal(void) {
    // Nothing to do
}

#else
static struct termios orig_termios;

void enableRawMode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

void disableRawMode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

int pollKey(void) {
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int rv = select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout);
    if (rv > 0) {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) == 1)
            return ch;
    }
    return -1;
}

void initializeTerminal(void) {
    enableRawMode();
}

void cleanupTerminal(void) {
    disableRawMode();
}
#endif