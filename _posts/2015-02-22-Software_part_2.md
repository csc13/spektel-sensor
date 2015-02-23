---
layout: post
title: "Firmware Part 2"
description: "Doing the measurements"
date: 2015-02-22
category: articles
tags: [Atmel, AVR, microcontroller, XMEGA, XMEGA32E5, firmware]
image:
  feature: Board_REV_B.jpg
---

Last year I promised to give a deeper look in the calculations. So to learn about the start 
of a measurement go back to [Firmware Part 1](/articles/Software_part_1).

###ADC base values
Getting the XMEGA datasheet Analog Digital Converter part, it says that if running in single ended unsigned mode, the
measurement value for GND is 205. This is to be able to measure a 0V voltage without problems.
To get the exact value was a problem. By measurement it was around 160. So I implemented
a self calibration (triggered by the button) to get the exact value at 0V. This variable
is named `adc_usig_base` and read from EEPROM.

The next needed value is the resolution for each bit (called Least Significant Bit).
For 12bit resolution this is 0.4899mV/LSB (constant `ADC_MV_12RES`).

###Measuring the voltage
Is easy and just uses a voltage divider of 15.24 ((4.7K + 330) / 330) (constant `MAIN_RES_DIV`)
to get the voltage from up to 26V to the measurement range of the XMEGA up to Vcc/1.6 = 2.06V 
in the single ended mode with 12bit resolution.

The calculation looks like the following:

{% highlight html linenos %}
uint16_t calc_main_mV(uint16_t res) {
	if( res <= adc_usig_base ) { 
		return 0; //clipping if the input is less than adc_usig_base
	}
	else {
		double _v = (double)((double)(res - adc_usig_base) * (double)ADC_MV_12RES * (double)MAIN_RES_DIV / 10);
		return (uint16_t)_v;
	}
{% endhighlight %}

We substract the `adc_usig_base` to get the correct range and multiply the result with the 12bit 
resolution to get mV. Then we need to multiply with the voltage divider to get the correct voltage.
In this case we divide by 10 to get 10mV as the result unit as needed for the Powerbox voltage
sensor.

###Measuring the current
The main task of the sensor is to read and transmit the current of the main battery pack.
Depending on the current running through the ACS758 hall sensor it outputs a voltage. This
voltage depends on the operating voltage Vcc and the current. The ACS758 can be operated
up to 5V and this is the voltage base in the datasheet. So we need to adjust all datasheet
values be a factor of 3.3V/5V = 0.66. Many values differ by the device, so all constants are
defined in the conf_board.h file (eg. to be able to change to a 200A version).

###ACS758 base values
We use the 100U version with Vcc 3.3V. So we get the following values:

- Voltage at Ip = 0A (no current) = 0.6V * 0.66 (for Vcc = 3.3V) = 0.396V (constant `ACS758_BASE`)
- Sensitivity for 1A = 40mV at Vcc = 5V
- Sensitivity for 1A at Vcc = 3.3V = 26.4mV (by measuring its only 24mV) (constant `ACS758_RATE`)
- Voltage at Ip = 100A = 0.0264V * 100 + 0.396V = 3.036V

That is close to the upper power rail of Vcc = 3.3V and to much for the XMEGA ADC. This
can measure up to Vcc/1.6 = 2.06V in the single ended mode with 12bit resolution. So we need to
apply a voltage divider to bring the Vout from the ACS758 to under 2.06V.

A voltage divider by 1.64 will do. I used a pair of 3K and 4.7K. This is calculated by
(3K + 4.7K) / 4.7K = 1.638. This looks pretty exact, but don't forget the resistors have
tolerances by 1-2%. This is the constant `CUR_RES_DIV`.

To get the measurement at 0A we can use the self calibration procedure as well. The variable
is called `adc_b` and stored in the EEPROM. 

###Calculate the current
The formular looks like this:
RES is the 12 bit result from the measurement.

`I = (((RES - ADC_USIG_BASE * ADC_MV_12RES * CUR_RES_DIV) - ACS758_BASE) / ACS758_RATE`

We substract the ADC base value for 0V and mutliply this with the resolution and 
voltage divider. From this voltage we substract the base voltage for 0A from the sensor and
multiply with the sensitivity to get the current. Then with some mathematic transformations:

`I * ACS758_RATE = ((RES - ADC_USIG_BASE) * ADC_MV_12RES * CUR_RES_DIV) - ACS758_BASE`

`I * ACS758_RATE / ADC_MV_12RES / CUR_RES_DIV = (RES - ADC_USIG_BASE) - (ACS758_BASE / ADC_MV_12RES / CUR_RES_DIV)`

`I = (RES - ADC_USIG_BASE) - (ACS758_BASE / ADC_MV_12RES / CUR_RES_DIV) * (ADC_MV_12RES * CUR_RES_DIV / ACS758_RATE)`
	 
var: `ACS_B = (ACS758_BASE / ADC_MV_12RES / CUR_RES_DIV)`

var: `ACS_R = (ADC_MV_12RES / ACS758_RATE * CUR_RES_DIV)`
	 
`I = (RES - ADC_USIG_BASE - ACS_B) * ACS_R`
	 
var: `ADC_B = (ADC_USIG_BASE + ACS_B)`
	 
`I = ((RES - ADC_B) * ACS_R`

`ACS_B` and `ACS_R` can be precalculated to save time. `ADC_B` can be measured through self 
calibration and is the same as `adc_b`.

`ACS758_RATE` will result in A. But for the telemetrie we want the result in 10mA or mA for
the capacity measurement. This is done with `_M1` for 10mA and `_M` for mA.

###Problems with the calculation
The first problem is, that I use unsigned integer. But this is bad, if the there is some 
drift and the measurement goes under `ADC_B`. This can be solved by clipping all results
below this value and simply return 0A.

The next problem is, that `ACS_R` will be a floating point number like 0.0312. If we
switch to 32bit integer and multiply all numbers by a factor x, so the 32bit field will
not be exceeded, this will lead to greater precision of the calculation.
At the end of the calculation we then divide again and get the correct value. If we
multiply by an exponent of 2, we can divide with a simple and fast shift operation.
To calculate the maximum factor in the 32bit limit I used an Excel sheet, you will
find in the Github repository. The calculation the looks like:

`I = (((RES - ADC_B) * ACS_R_O) >> ACS_SHIFT`

`ACS_SHIFT` = 5 divides by 32 for example.

In the calc_measure.c the whole thing looks like this:

{% highlight html linenos %}
uint16_t calc_mA(uint16_t res) {
	if( adc_b >= res ) { 
		return 0; //clipping to zero
	}
	uint32_t _c = (( (uint32_t)( ((uint32_t)(res - adc_b)) * ((uint32_t)ACS_R_M1_O) ) ) >> ACS_SHIFT_M1);
	return ((uint16_t)_c);
}
{% endhighlight %}

In this case the function returns the current in 10mA like required for the Spektrum
telemetry Current sensor.

In the next round I will explain the capacity measurement. Then we need to add the time
component to the current measurement.











