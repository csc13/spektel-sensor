---
layout: post
title: "Adding Processing support"
description: "Using the serial interface with Processing"
category: articles
tags: [Processing, telemetry, Arduino, plotter, serial]
image:
  feature: Prototype_REV_A.jpg
---

#### 2014-11-02: Added support for RealtimePlotter and arduplot
I learned, that the main part on building a sensor is the software to grind and polish the
signals read from the hardware sensor. In this case it was the Allegro ACS758LCB-100U hall 
effect current sensor.

So I read about digital filtering and use 3 cascaded the `Moving Average` algorithm to smooth the
sensor reading.

{% highlight html linenos %}
//filter use in main.c
AddToFloatAvg(&cur_filter1, (cur_mea_val));
AddToFloatAvg(&cur_filter2, GetOutputValue(&cur_filter1));
AddToFloatAvg(&cur_filter3, GetOutputValue(&cur_filter2));
cur_adc_res[act] = (GetOutputValue(&cur_filter3));
{% endhighlight %}

Each filter has 9 readings. For details see `src/floating_average.h` and `.c` files.

By testing I used the DX9 transmitter, recorded to the SD card and read that using the TLM Reader.
This was a long procedure. Since I already put a header with the Rx, Tx serial pins on the
board it shouldn't be that much of a problem to show some output on the Mac using the Processing
tools.

I found two great serial monitors based on [Processing](https://www.processing.org/).
Please see the [RealtimePlotter](https://github.com/sebnil/RealtimePlotter.git) and 
[Arduplot](https://github.com/dahart/arduplot.git). Thanks to their contributors. Just uncomment
the lines in the `config/conf_board.h` file.