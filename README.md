# Adsbdec
An ADSB open source decoder for the airspy R2 and mini

## Usage 
> adsbdec [-d] [-e] [-f filename] [-s addr[:port]]

By default receive samples from airspy and output long adsb frames (DF14/17/18/20/21/24) in raw avr format on stdout

## Options
	-d : output short frame frames too
	-e : use 1bit error correction
	-f : input from filename instead of airspy (raw signed 16 bits REAL format 20Mb/s)
	-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)

## Example

	On computer connected to airspy :
> adsbdec -e -s 192.168.0.10:30001

	On another computer at addr 192.168.0.10 :
> dump1090 --net-only --net-ri-port 30001 

Man could use adsbdec to send data to any other avr format compatible server (VRS, feeders for main adsb web site, etc )
For VRS select "Push receiver" , "AVR format"

You could use netcat , if you need to act as server :
> adsbdec -e | netcat -l 127.0.0.1 30002

## Compile

   Need libusb and libairspy

### For airspy R2 

	Just do
> make


### For airspy mini

	Edit Makefile to add -D AIRSPY_MINI (follow instructions in Makefile), then 
> make

