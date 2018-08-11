CFLAGS= -Wall -Ofast -pthread -I.  `pkg-config --cflags libairspy`
LIBS= -lm -pthread  `pkg-config --libs libairspy` -lusb-1.0


adsbdec: demod.o main.o crc.o output.o air.o
	$(CC) $(LDFLAGS) demod.o main.o crc.o output.o air.o $(LIBS) -o adsbdec

clean:
	rm *.o adsbdec
