CFLAGS= -Wall -Ofast -march=native -pthread -I.  `pkg-config --cflags libairspy`

LIBS= -lm -pthread  `pkg-config --libs libairspy libusb` 

adsbdec: demod.o main.o crc.o output.o air.o valid.o
	$(CC) $(LDFLAGS) demod.o main.o crc.o output.o air.o valid.o $(LIBS) -o adsbdec

install: adsbdec
	sudo cp adsbdec /usr/local/bin

uninstall: 
	sudo rm /usr/local/bin

clean:
	rm *.o adsbdec
