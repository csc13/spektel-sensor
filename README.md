spektel-sensor
==============

This is a small project for a ATXMEGA32E5 based sensor for the Spektrum-RC telemetry system.
![spektrum Sensor Prototype](https://github.com/csc13/spektel-sensor/blob/master/pictures/Prototyp.jpg)

Basic functions are:
- Show used capacity of main battery
- Show actual current of main battery
- Show actual voltage of main battery

### Planned functions are
- Show altitude
- Show climb rate
- Show cell voltages of main battery
- Make it somehow programmable (change the current alarm for example)

### Project Structure
- firmware (compiled firmware for XMEGA)
- flight_logs (from Spektrum transmitter)
- hardware (Eagle PCB files)
- pictures (photos)
- project (Atmel Studio, make files, etc.)
- src (C sources)

### History
#2014-10-28: Initial upload with REV_A board layout
- Please note that REV_A is the initial prototype and has several bugs

#2014-11-02: Added support for RealtimePlotter (see https://github.com/sebnil/RealtimePlotter.git ) 
	and arduplot (see https://github.com/dahart/arduplot.git), two great serial monitors
	based on Processing (https://www.processing.org/).
