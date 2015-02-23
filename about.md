---
layout: post
permalink: /about/index.html
title: About this project
description: This is a small project for a XMEGA based sensor for the Spektrum-RC telemetry system
tags: [XMEGA, AVR, sensor, SPEKTRUM-RC, RC, telemetry]
image:
  feature: Prototype_REV_A.jpg
---

### Why this blog
For this project and many other things I'm using the free contributions of many fine people 
publishing their work to blogs, forums and other places. I like forums, and there are great
people offering their help, but sometimes it took me many hours reading discussions 
coming to the point. What I like most is an explaining web site and all the code in a
 repository like GitHub. So that is just what I try to do here. 

Despite I'm not an english native speaker, I will post in english to make this information
accessible to as many as possible.

Thanks to all direct and indirect contributors to this project.


### Why this project
I was always interested in how things work and to apply my knowledge. So I'm almost restless 
in thinking of new ideas and how to make them on my own. I used microcontroller modules and
used to solder, but never designed a PCB or get in the very details of a micro. One of my
targets of this project was to learn a lot about these things.

I'm a RC Hobbyist hooked to RC helicopters owning some Spektrum-RC equipment.
Spektrum-RC announced a capacity sensor for some time,
but it's still a miracle. There are capacity sensors available by the RC system manufacturers
or by 3rd party companies, but not for Spektrum-RC. There are many projects, but non I found
offers all informations for an easy rebuild.

**EDIT 2015-02-23**: Spektrum RC released their capacity sensor! And provided updates for the
transmitters for this new Flight pack capacity sensor (this project supports this new
sensor display as well). Then the JLog 2.6 from [R²prototyping GmbH](http://www.r2prototyping.ch/?page_id=709)
has support for Spektrum RC (great engineering).

So nobody needs to build a DIY sensor. But you can, if you want!

### The project setup
The sensor is based on the flowing components:
- Atmel XMEGA32E5
- Allegro ACS758LCB-100U hall effect current sensor
- Bosch BMP180 pressure sensor

The firmware was developed in C using Atmel Studio 6 and the Atmel ASF framework.

	Note: The Spektrum RC TM1000 telemetry modul, a DSMX receiver with data port and a 
	transmitter with telemetry functions are required to use the sensor. Not all Spektrum RC 
	telemetry able transmitters support all sensors. I use a DX9 for testing.

### Basic functions
The sensor measures 25 times a second the actual current, voltage and pressure. From this 
it calculates the used capacity, hight and climb rate.

- Show used capacity of main  battery
- Show actual current of main battery
- Show actual voltage of main battery

### Advanced functions
- Show altitude (done)
- Show climb rate (open)
- Show cell voltages of main battery (maybe in the future)
- Make it somehow programmable (change the current alarm for example) (done)

### GitHub project structure
- firmware (compiled firmware for XMEGA)
- flight_logs (from Spektrum transmitter)
- hardware (Eagle PCB files)
- pictures (photos)
- project (Atmel Studio, make files, etc.)
- src (C sources)

### Thanks
This project mostly profited from Mukenukem contributions on the Spektrum-RC telemetry
protocol on rcgroups.com. Without this incredible research the project wouldn't be here.
But there are many other successful projects out, which inspired me. Thanks to them as well.

### About me
I was born in the early seventies in Germany and working as an IT professional on the
business consulting side. But I started my career as an engineer and programmer.
So this hobby project is a little bit back to the roots to me.