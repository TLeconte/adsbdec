# Adsbdec
An ADSB open source decoder for the airspy R2 

## Usage 
> adsbdec [-a] [-b] [-m] [-g gain] [-f filename] [-s addr[:port]]

By default receive samples from airspy and output adsb frames (DF11/DF14/17/18/20/21) in raw avr format on stdout

## Options
	-a : output ground frames too
	-m : output avrmlat format (ie : with 12Mhz timestamp)
	-b : output binary beast format
	-g 0-21 : set linearity gain 
	-f : input from filename instead of airspy (raw signed 16 bits real format)
	-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)
	-l addr[:port] : listen to addr:port (default port : 30002) and accept a TCP connection where to send output 

Man could use adsbdec to send data to any other avr,avrmlat  or beast format compatible server (VRS, readsb, feeders for main adsb web site, etc )

## Example

For VRS select "Push receiver" , "AVR format", and :
> adsbdec -s 192.168.0.10:30001

For a local readsb with beast input select  :
> adsbdec -b -s  127.0.0.1:30104

If you need that adsbdec act as server (like dump1090) :
> adsbdec -l 192.168.0.10:30002

## Compile

   Need gcc cmake libusb-dev and libairspy-dev 

  Just do 

> mkdir build ; cd build
  
> cmake .. ; make ; sudo make install


