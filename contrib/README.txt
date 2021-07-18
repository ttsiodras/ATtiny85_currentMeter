This code uses the magnificent ImageMagick to convert the amstrad_cpc464.ttf
into a PGM containing the font data as a simple black/white image.

I only care about ASCII chars from 32 to 127; so I use "alphabet.txt" as
the driving source for ImageMagick. My brain-dead "make_header.c" will then
load the created image and create an array definition ("amstrad.cpp") whose
data I then manually copied inside ../SSD1306_minimal.cpp (see AmstradFont
array). Since the code of SSD1306_minimal indexes via the character code,
I also add copy 32 lines of zeroes at the top of the array.

After that, I just updated printChar to use the 8 bytes of the font definition
(instead of the 5 in the default (too tiny!) 5x7 font.
