CFLAGS= -Wall -Ofast -pthread -I.  `pkg-config --cflags libairspy`
# For airspy mini comment the previous line and uncomment the following one
#CFLAGS= -Wall -Ofast -D AIRSPY_MINI -pthread -I.  `pkg-config --cflags libairspy`

LIBS= -lm -pthread  `pkg-config --libs libairspy` -lusb-1.0


adsbdec: demod.o main.o crc.o output.o air.o valid.o
	$(CC) $(LDFLAGS) demod.o main.o crc.o output.o air.o valid.o $(LIBS) -o adsbdec

clean:
	rm *.o adsbdec
