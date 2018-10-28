# Adsbdec
An ADSB open source decoder for the airspy R2 and mini

## Usage 
> adsbdec [-d] [-c] [-e] [-m] [-g 0-21] [-f filename] [-s addr[:port]]

By default receive samples from airspy and output long adsb frames (DF14/17/18/20/21/24) in raw avr format on stdout

## Options
	-d : output short frames too
	-c : only frame with true crc (DF11/17/18)
	-e : use 1 bit error correction
	-m : output avrmlat format (ie : with 12Mhz timestamp)
	-g 0-21 : set sensibility gain (default 21)
	-f : input from filename instead of airspy (raw signed 16 bits real format)
	-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)

## Example

> adsbdec -e -s 192.168.0.10:30001

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

