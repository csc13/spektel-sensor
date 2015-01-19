spektel-sensor
==============
*[See the project blog here!](http://spektel.cescholz.de/)* 

This is a small project for a XMEGA based sensor for the Spektrum-RC telemetry system.
![spektrum Sensor Prototype](https://github.com/csc13/spektel-sensor/blob/master/pictures/Prototyp.jpg)

The sensor is based on the flowing components:
- Atmel XMEGA32E5
- Allegro ACS758LCB-100U hall effect current sensor
- Bosch BMP180 pressure sensor (planned)

The firmware was developed in C using Atmel Studio 6 and the Atmel ASF framework.

	Note: The Spektrum RC TM1000 telemetry modul, a DSMX receiver with data port and a 
	transmitter with telemetry functions are required to use the sensor. Not all Spektrum RC telemetry able
	transmitters support all sensors. I use a DX9 for testing.

### Basic functions
The sensor measures 25 times a second the actual current, voltage and pressure. From this 
it calculates the used capacity, hight and climb rate.

- Show used capacity of main battery
- Show actual current of main battery
- Show actual voltage of main battery
- Show altitude
- Programmable via on button (change the battery capacity)

### Planned functions
- Show climb rate
- Show cell voltages of main battery


### Project Structure
- firmware (compiled firmware for XMEGA)
- flight_logs (from Spektrum transmitter)
- hardware (Eagle PCB files)
- pictures (photos)
- project (Atmel Studio, make files, etc.)
- src (C sources)

### History
#### 2014-10-28: Initial upload with REV_A board layout
- Please note that REV_A is the initial prototype and has several bugs

#### 2014-11-02: Added support for RealtimePlotter and arduplot
Two two great serial monitors based on Processing (https://www.processing.org/).
Please see https://github.com/sebnil/RealtimePlotter.git and see https://github.com/dahart/arduplot.git).

#### 2015-01-19: Added support for the REV_B board
This includes the smaller board, BMP180 altitude measurement, setup button, self calibration.

