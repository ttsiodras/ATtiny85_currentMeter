CFLAGS+=-g

all:	make_header
	convert -font amstrad_cpc464.ttf -pointsize 8 label:"@alphabet.txt" x.png
	convert  x.png  x.pgm
	rm -f x.png
	./make_header
	echo "Now copy amstrad.cpp into the array inside ../SSD1306_minimal.cpp"
	echo "Remember to copy 32 lines of zeroes at the top!"

make_header: make_header.o
	gcc -o $@ -g $<

%.o:	%.c
	gcc -c ${CFLAGS} -o $@ $<

clean:
	rm -f make_header make_header.o x.pgm amstrad.cpp
