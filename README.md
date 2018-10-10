# Adsbdec
An ADSB open source decoder for the airspy

## Usage 
> adsbdec [-d] [-e] [-f filename] [-s addr[:port]]

By default receive samples from airspy and output DF17/18 adsb frames in raw avr format on stdout

## Options
	-d : output short frame frames too
	-e : use 1bit error correction (more frames, more cpu, more  false positive frames)
	-f : input from filename instead of airspy (raw signed 16 bits IQ format)
	-s addr[:port] : send ouput via TCP to server at address addr:port (default port : 30001)

## Example

	On computer connected to airspy :
> adsbdec -d -e -s 192.168.0.10:30001

	On another computer at addr 192.168.0.10 :
> dump1090 --net-only --net-ri-port 30001 

Man could use adsbdec to send data to any other avr format compatible server (VRS, feeders for main adsb web site, etc )

You could use netcat , if you need to act as server :
> adsbdec -e | ncat -l 127.0.0.1 30002

## Compile

	Just do make.
	Need libusb and libairspy
