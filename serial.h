#ifndef SERIAL_H
#define SERIAL_H

// Terminal handling functions
void initializeTerminal(void);
void cleanupTerminal(void);
void enableRawMode(void);
void disableRawMode(void);
int pollKey(void);

#endif // SERIAL_H
