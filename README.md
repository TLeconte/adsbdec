# Adsbdec
An ADSB open source decoder for the airspy R2 and RTLSDR

## Usage 
> adsbdec [-a] [-b] [-e] [-m] [-g gain] [-f filename] [-s addr[:port]]

By default receive samples from airspy and output adsb frames (DF11/DF14/17/18/20/21) in raw avr format on stdout

## Options
	-a : output short frames too
	-e : use 1 bit error correction
	-m : output avrmlat format (ie : with 12Mhz timestamp)
	-b : output binary beast format
	-g 0-21 : set linearity gain (for rtlsdr -g in tenth of db (ie : -g 450))
	-f : input from filename instead of airspy (raw signed 16 bits real format)
	-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)
	-l addr[:port] : listen to addr:port (default port : 30002) and accept a TCP connection where to send output 

Man could use adsbdec to send data to any other avr format compatible server (VRS, feeders for main adsb web site, etc )

## Example

For VRS select "Push receiver" , "AVR format", and :
> adsbdec -e -s 192.168.0.10:30001

If you need that adsbdec act as server (like dump1090) :
> adsbdec -e -l 192.168.0.10:30002

## Compile

   Need libusb and libairspy or librtlsdr

  Just do 

> mkdir build ; cd build 
> cmake -Dairspy=ON  OR -Drtl=ON ..
> make
