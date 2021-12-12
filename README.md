# Adsbdec
An ADSB open source decoder for the airspy R2

## Usage 
> adsbdec [-d] [-c] [-e] [-m] [-g 0-21] [-f filename] [-s addr[:port]]

By default receive samples from airspy and output long adsb frames (DF14/17/18/20/21/24) in raw avr format on stdout

## Options
	-d : output short frames too
	-c : only frame with true crc (DF11/17/18)
	-e : use 1 bit error correction
	-m : output avrmlat format (ie : with 12Mhz timestamp)
	-b : output binary beast format
	-g 0-21 : set linearity gain 
	-f : input from filename instead of airspy (raw signed 16 bits real format)
	-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)
	-l addr[:port] : listen to addr:port (default port : 30002) and accept a TCP connection where to send output 

Man could use adsbdec to send data to any other avr format compatible server (VRS, feeders for main adsb web site, etc )

## Example

For VRS select "Push receiver" , "AVR format", and :
> adsbdec -e -s 192.168.0.10:30001

To directly feed to adsbexchange (without mlat) :
> adsbdec -b -s feed.adsbexchange.com:30005

If you need that adsbdec act as server (like dump1090) :
> adsbdec -e -l 192.168.0.10:30002

## Compile

   Need libusb and libairspy

  Just do
> make
