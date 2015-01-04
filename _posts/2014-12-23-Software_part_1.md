---
layout: post
title: "Firmware Part 1"
description: "Start with the Software"
date: 2014-12-23
category: articles
tags: [Atmel, AVR, microcontroller, XMEGA, XMEGA32E5, firmware]
image:
  feature: Board_REV_B.jpg
---
After the board is assembled and working it is time to move to the software part. As
mentioned earlier, most of the magic is in the code. This post is intended to help to
understand the source code of the sensor firmware.

This is not a C or Atmel Studio tutorial. It will suite programmers, who know basic C (or
C style programming) and move to the XMEGA development. Although the Atmel datasheet is our
friend and the basis of the code I will not go into the XMEGA details. I'm just not expert 
enough up to now to do so.

Download the XMEGA datasheet and manual from the [Atmel](http://www.atmel.com) website. 
The datasheet covers the specialities of the `ATXMEGA32E5` while the manual is the same
for all XMEGAs and provides a deep look in each hardware module.

###Testing the board
The "Hello world!" in the µC world is a blink. The board has two LEDs to do some blinking
and see if the code uploads without problem. Whenever you get stuck it's time to reassure
the basic functions. A Todo would be to provide some basic testing code. Maybe by pressing
the button, the sensor can run some test, blink the LEDs, send some test data to the transmitter.

###Development process
I use the Atmel studio 6 with the ASF framework. The ASF wizard helps to specify the
required libraries based on the cpu modules and functions you want to use. By setting up 
the right device (the ATXMEGA32E5) and a custom board it selects the correct pin and register
definitions.
After writing the code, it needs to be compiled in an .elf file containing the machine code.
Using the programming tool in AVR Studio the flash memory gets erased and rewritten. The
µC resets and starts to run the new code.

A debugger seems very useful, but I didn't use it much. In most environments a debugger
affects the runtime, at least it corrupts the timing. I don't know the effects for
XMEGA debugging at the moment.

###Basic architecture of the firmware
The software consist of three main blocks:

A.) The classic `while(1){...}` in the `main()` function doing most of the calculation. After
entering the infinit while loop the `sleepmanager`is called and goes to sleep, until it
gets a wake up call by block B (an additional global variable is used to assure only block B
can trigger main measurement). The calculated measurements are written to block C.

B.) A timer to trigger the main ADC measuring the current using the Allegro hall sensor.
The ADC is coupled to the timer by the event system. After the measurement is finished, an
interrupt is fired reading the result und waking up block A from the sleep mode.

C.) This block serves the X-Bus requests from the Spektrum TM1000 telemetry module. This
block is triggered by a TWI/I2C interrupt and just uses the values provided by block B 
and transmits them to the telemetry module.

So we have two interrupt sources, the main measurement timer and the X-Bus interrupts. The
first triggers the measurements and calculations. The X-Bus trigger grabs the data and
sends it out. Now we go through the programm flow starting with the timer trigger of block
B. 

###Triggered by a timer
A timer is a module, which can fire events after a defined number of clock cycle counts 
or when the counter register gets an overflow. The XMEGA can be clocked up to 32MHz, this
are 32 million counts per second, if not using any prescaler. A prescaler can divide the
clock rate by a binary factor (2,4,8,16, etc.) for longer intervals. By counting the 
overflows in a second register timers can be used for longer times, than the overflow time.

The basic trigger of the sensor is a timer at 25 or 50Hz firing an event every 40 or 20ms.
Both settings work without problems. To get the right time, many parameters needs to be
considered. If the time is to long, the measurement may be to rough for a good capacity
calculation. If the time is to short, the ADC measurement and calculations may be interrupted
by the next cycle. ADCs need some time for precise results. The `tc_init()` function
initializes the timer with 3 interrupts. The `error_interrupt` catches errors in the timer
system and lights the red LED. the `overflow_interrupt` restarts the timer. The `cca_interrupt`
is the one we are interested in this case. It is set to fire at half the timer period. Maybe
for clarity I could have changed the code to just use the `overflow_interrupt`. This would
have the same effect in this case. 

The timer event is used to trigger the event system. By using the event system of the XMEGA
events like the start of an ADC measurement can be started. So more than one event can be
triggered by one interrupt and it gives additional flexibility. It's not necessary in this
code, but I used it nevertheless. The following lines configure the event system:

{% highlight html linenos %}
EVSYS.CH3MUX = EVSYS_CHMUX_TCC4_CCA_gc;  //Connect TCC4 Compare interrupt to event channel 3 (used to trigger ADC)
tc45_set_resolution(&TIMER_SENS, TIMER_SENS_RESOLUTION);
{% endhighlight %}

**Please note**, that several config files, typically named `conf/<modulname>_config.h` are used
to parameterize the modules on startup. These are used by the modules init functions.

###Analog-Digital-Converter
The heart of the sensor is the Allegro hall sensor converting a current up to 100 Amps (or
even up to 200A with another version) to an almost linear voltage and the XMEGA analog to
digital converter. The XMEGA ADC can convert a voltage range between GND and around 2V to
a 12Bit value in single-ended unsigned mode. This will be reduced by using a signed mode
(for negative voltages) or an external voltage reverence. Using the 12Bit the least 
significant bit represents 0.4899mV.

The Allegro outputs 396 to 410 mV by 0mA current up to nearly 3.1V at 100A. So we have 
to transfer this range to the max of 2.048V of the external voltage reference or the internal
voltage reference of Vcc/1.6 = 2.063V. This is done by a voltage divider of 3K to 4.7K Ohm.

The ADC gets initialized in `adc_init()`. There is the ADC port and an ADC channel. The used
XMEGA has only one ADC port and one channel. Multiple channels would allow for parallel
measurements. This would have been nice for measuring the current and voltage simultaneously.

Setting up the ADC:
{% highlight html linenos %}
adc_set_conversion_parameters(&adc_conf, ADC_SIGN_OFF,ADC_RES_MT12 , ADC_REF_VCC);  // ADC_RES_12
{% endhighlight %}
Sets the unsigned mode with 12Bit resolution and the internal Vcc as VREF.

{% highlight html linenos %}
adc_set_conversion_trigger(&adc_conf, ADC_TRIG_EVENT_SYNCSWEEP,1, 3 );
adc_set_clock_rate(&adc_conf, ADC_CLOCK); // ADC clock 1.8MHz
{% endhighlight %}
Sets the trigger to an event and specifies the ADC clock rate. Higher clock rates allow for
faster measurements, slower is more precise. I use a fast setting to be save.

{% highlight html linenos %}
adc_set_callback(&ADC_MAIN, *adc_cur_callback);
{% endhighlight %}
Callback function called when the measurement is finished. The time of a measurement is 
calculated by the number of result bits (at least one clock cycle per bit) and some time 
to transfer the result to the result register.  

To use different pins for ADC probes, the ADC channnel needs to be reconfigured. So there
are two functions to set up the channel. The `adcch_set_cur_measure()` for current and
`adcch_set_volt_measure()` for the main battery voltage measurement. The difference is, that
the current measurement works with an interrupt firing after the result is there, while
the voltage measurement gets started and then checked (blocking in a while loop) for the
result.

Setting up the ADC channel:

{% highlight html linenos %}
adcch_set_input(&adcch_conf, ADC_MAIN_CURRENT_PIN, ADCCH_NEG_NONE, 1);
{% endhighlight %}
This sets the pin for measuring, the negative pin which is not used here and gain (not used here).

{% highlight html linenos %}
adcch_enable_averaging(&adcch_conf, ADC_CUR_AVERAG_SAMP);
{% endhighlight %}
I enabled the averaging function of the XMEGA32E5 taking 4 samples and returning the average
of these.

{% highlight html linenos %}
adcch_set_interrupt_mode(&adcch_conf, ADCCH_MODE_COMPLETE); //complete conversion
adcch_enable_interrupt(&adcch_conf);    
{% endhighlight %}
Configures the channel interrupt to fire when the analog to digital conversion is completed.
The callback function is taken from the ADC setup.

If the ADC conversion is completed, the callback function is called:

{% highlight html linenos %}
static void adc_cur_callback(ADC_t *adc, uint8_t ch_mask, adc_result_t res) {
	cur_mea_val = res;
	act = !act;
	time[act] = rtc_get_time();
	
	measure_cycle = true;	
}
{% endhighlight %}
Since this XMEGA provides only one channel, the channel mask parameter will alway be the same.
The result is saved to the `cur_mea_val` global variable. Then the actual time is saved
to `time[act]`. This is switched each measure cycle to have the actual and latest measurement
time for the capacity calculation. The measure_cycle = true is set to signalize the main
loop that a new result is available.

I will cover the main calculations in the next part!