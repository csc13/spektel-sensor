---
layout: post
title: "Initial Upload"
description: "This was the starting point for the GitHub repository!"
date: 2014-10-28
tags: [XMEGA, AVR, sensor, SPEKTRUM-RC, RC, telemetry]
image:
  feature: Prototype_REV_A.jpg
---
The prototype of the spektel-sensor passed its first successful tests. The revision A layout
and corresponding code is up on [GitHub](https://github.com/csc13/spektel-sensor.git).

Please be aware, that `REV_A` had several faults on the board. So the Bosch BMP180 sensor
won't work, because I didn't read the datasheet correctly.

After learning Eagle PCB, a software I can recommend to everybody, I spend some nights to
find the Eagle libraries for the parts I wanted to use and create the layout. At the end
I was little impatient to order the PCB (printed circuite board) at a board manufacturer.
I chose a little one, who did a great job etching and drilling the board.

But I found several bugs in the layout. So I used a exacto hobby knife, some lacquered copper
wire and fixed it. As a mostly software guy I had to learn that a hardware bug can't be fixed that
fast. And ordering a new board takes some days and money. so in the future I will print the board
layout on paper and go through all components and wires.
