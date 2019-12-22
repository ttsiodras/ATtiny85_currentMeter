#include "Emiter.h"

//////////////////////////////////////////////////////////////
// Optimizing for size...
//////////////////////////////////////////////////////////////
// I need to cleanup the msgBuf with spaces everytime I print,
// to avoid having to cleanup the entire screen every time.
//
// Initially I just used my SAFE_PRINTF macro to emit a memset
// followed by a drawString; but that meant I was wasting `.text`
// space, of which the tiny brain of ATtiny85 has only 8K of...
//
// So instead of going full varargs, I used compile-time cpp-based
// __VA_ARGS__, coupled with constructor/destructor semantics
// that are re-using the same memset/drawString calls every time.
// 2.7%  improvement in .text usage :-)

Emiter::Emiter() {
    reset();
}

void Emiter::reset()
{
    memset(msgBuf, ' ', sizeof(msgBuf));
    msgBuf[sizeof(msgBuf) - 1] = '\0';
    columnNo = 0;
}

void Emiter::printChar(char c) 
{
    if (columnNo < sizeof(msgBuf)-1)
        msgBuf[columnNo++] = c;
}

void Emiter::printString(const char *src) 
{
    size_t i, n=sizeof(msgBuf)-columnNo-1;
    for (i=0; i<n && src[i] != '\0'; i++)
        msgBuf[columnNo + i] = src[i];
    msgBuf[columnNo + i] = '\0';
    columnNo += i;
}

void Emiter::printInt(
    int i,             // input value
    bool sign,         // whether the value can be negative (i.e. %d vs %u)
    bool padded,       // whether the value should be padded
    int padwidth,      // number of digits to pad (%3d)
    int base,          // what's the numeric base i.e. %d (10) vs %x (16)
    int letbase)       // for hex nibbles (%x or %X) what is the baseline character
{
    static char print_buf[sizeof(msgBuf)];
    char *s;
    int pc = 0;
    bool neg = false;
    unsigned int u = i;

    if (i == 0) {
        i = padded?padwidth:1;
        do { printChar('0'); } while(--i);
        return;
    }

    if (sign && base==10 && i<0) {
        neg = true;
        u = -i;
    }

    s = print_buf + sizeof(msgBuf) - 1;
    *s = '\0';

    while (u) {
        int t = u % base;
        if (t >= 10)
            t += letbase - '0' - 10;
        *--s = t + '0';
        pc++;
        u /= base;
    }

    if (neg) {
        *--s = '-';
        pc++;
    }

    if (padded)
        while(pc++ < padwidth)
            printChar('0');

    printString(s);
}

Emiter::~Emiter()
{
    // flush();
}

void Emiter::flush()
{
    msgBuf[sizeof(msgBuf)-1] = 0;
    u8x8.drawString(0, lineNo++, msgBuf);
    columnNo = 0;
}

char Emiter::msgBuf[32] = {0};
int Emiter::lineNo = 0;
unsigned Emiter::columnNo = 0;
