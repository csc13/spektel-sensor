---
layout: post
title: "Firmware Part 3"
description: "Capacity measurement"
date: 2015-02-23
category: articles
tags: [Atmel, AVR, microcontroller, XMEGA, XMEGA32E5, firmware]
image:
  feature: Board_REV_B.jpg
---

In the last [post](/articles/Software_part_2) I explained the calculation of the current 
using the ADC measurements. Now we come to the calculation of the used capacity.

###What we need
We want to get the battery capacity used in something like mAh. This is near to the used
energy if we do the incorrect assumption, that the voltage is fixed. But for the Ampere-hour
we don't need the voltage.

<figure>
	<img src="/images/ampere-hour.png">
	<figcaption>We need to get the ampere-hour</figcaption>
</figure>

What we want to calculate is the red area under the current curve. This is an integral
over the time.

###What we have
The sensor measures the current in timer triggered intervals (in our case 40ms, but the
time is variable in the calculation).

<figure>
	<img src="/images/digitalized_current_curve.png">
	<figcaption>The digitalized current curve</figcaption>
</figure>

We store the actual measurement `res2` and time of the measurement `time2` and the 
measurement before `res1` and `time1`. After each calculation `res2` becomes `res1` and
a new measurement is done to `res2`.

###The calculation  
We need to get the surface of the blue tetragon. If we think of mirroring the same tetragon
on top this is a rectangle with one edge `res1 + res2` and the other `time2 - time1`.

<figure>
	<img src="/images/integral_Ah.png">
	<figcaption>Integrating over time</figcaption>
</figure>
This can be easily calculated.

`Ah = (res1 +res2) * (time2 - time 1) / 2`
or
`Ah = (res1 +res2) / 2 * (time2 - time 1)`

Now this is combined with the current calculation of the last [post](/articles/Software_part_2).
We check for clipping. Then calculate the y-axis and subtracting `adc_b` for 0A.
Because we then divide by 2 (here a right shift by 1 (`>> 1`)) we need to double `adc_b`
(again done by a left shift). Then we multiply by the time period and use `ACS_R_M_O` 
and the correcting shift by `ACS_SHIFT_M` to get mAms (miliamps by miliseconds).

{% highlight html linenos %}
uint32_t calc_cap_mAms(uint32_t res1, uint32_t res2, uint32_t time1, uint32_t time2) {
	if( adc_b >= res1 ) res1 = adc_b; //clipping to zero
	if( adc_b >= res2 ) res2 = adc_b; //clipping to zero
	//for time in 1ms per bit
	return (((uint32_t)(((res1 + res2 - (adc_b << 1)) >> 1)  * (time2 - time1) * ACS_R_M_O)) >> ACS_SHIFT_M);	}
{% endhighlight %}

So every 40ms we get a very small chunk of mAms used capacity. But even if we have a 32bit
variable this is only 1193 mAh. Not enough for most models.

###Getting the result
To transform mAms to mAh we just need to divide the mAms by 3600,000. But if we do that
every measurement with the small chunks, we wont get any result.
So in the main.c the mAms are added and checked if its greater than 1 mAh = 3600,000 mAms.
If so we increase the used capacity in mAh by 1 and substract 3600,000 from the mAms variable.

{% highlight html linenos %}
//Check for capacity overflow
if( cap_mAms >= 3600000 ) {
	cap_mAms -= 3600000;  //- one mAh
	cap_mAh++;			  //+ one mAh
	flight_cap.cap = cap_mAh; //set capa used for flight pack capacity sensor
	power.cap1--;         //subtract from capacity output
	//Check for alarm: under BAT_MIN (for powerbox sensor only)
	if( power.cap1 <= cap_min ) {
			power.cap1_alarm = true;
	}
}
{% endhighlight %}

Then we set the flight_cap.cap for the Spektrum Flight Pack Capacity sensor and power.cap1
for the Powerbox Cap1 Sensor value (this is counted down). If power.cap1 goes under 20% of
the full battery capacity the Powerbox alarm is triggered.

This is it for the core firmware functionality. There are additional setup functions and
all the stuff around the BMP180 pressure / altitude sensor. I might or might not cover that
in a later post.

