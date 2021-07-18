#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

int main()
{
    FILE *fp = fopen("x.pgm", "rb");
    fseek(fp, 0, SEEK_END);
    int sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *p = malloc(sz);
    int sz2 = fread(p, 1, sz,  fp);
    assert(sz2 == sz);
    fclose(fp);

    int width = 761;
    int height = 9;
    
    FILE *fp2 = fopen("amstrad.cpp", "w");
    fprintf(fp2, "const unsigned char  AmstradFont[] PROGMEM = {\n");
    p += 13;
    for(int c=32; c<128; c++) {
        // for(int i=0; i<9; i++) {
        //     uint8_t bitdata = 0;
        //     for(int j=0; j<8; j++) {
        //         int pixel = p[width*i + j];
        //         putchar(pixel == 0xff ? ' ' : '@');
        //     }
        //     putchar('\n');
        // }
        // printf("====== This was: %c ====\n", c);

        fprintf(fp2, "    ");
        for(int j=0; j<8; j++) {
            uint8_t bitdata = 0;
            for(int i=8; i>=0; i--) {
                int pixel = p[width*i + j];
                bitdata<<=1;
                bitdata |= (pixel&1);
                putchar(pixel == 0xff ? ' ' : '@');
            }
            putchar('\n');
            bitdata ^= 0xFF;
            fprintf(fp2, "0x%02x, ", (unsigned) bitdata);
        }
        fprintf(fp2, "\n");
        printf("====== This was: %c ====\n", c);
        p += 8;
    }
    fprintf(fp2, "};");
}
