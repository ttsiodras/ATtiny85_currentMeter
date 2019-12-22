#ifndef __EMITER_H__
#define __EMITER_H__

struct Emiter {
    static char msgBuf[16];
    static int lineNo;
    static unsigned columnNo;

    // Clear buffer with spaces
    Emiter();

    // Emit constant string
    void printString(const char *src);

    // Emit integer
    void printInt(
        int i,             // input value
        bool sign=false,   // whether the value can be negative (i.e. %d vs %u)
        bool padded=false, // whether the value should be padded
        int padwidth=3,    // number of digits to pad (%3d)
        int base=10,       // what's the numeric base i.e. %d (10) vs %x (16)
        int letbase='a');  // for hex nibbles (%x or %X) what is the baseline character

    // Emit character
    void printChar(char c);

    // Draw final string
    void flush();
    void reset();

    ~Emiter();
};

#endif
