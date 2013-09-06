EZ430-Chronos-Firmware
======================

Fork of the original firmware with Google authenticator and pedometer.
The original firmware is the "eZ430-Chronos v1.1 - white PCB".

Google authenticator
--------------------
It comes from the work of [amd989](https://github.com/amd989/ezTOTP).

Look the link above for details on the configuration.

Pedometer
---------
It comes from [here](http://ez430chronos.blogspot.fr/2012/08/pedometer.html).
But I have changed some details:

* The pedometer appears on the bottom line
* There are 5 digits (counter up to 99999)
* When you hold (#), the counter is reinitialized
* When you press the down arrow, you switch between 3 display types (number of movements -> percent relative to a reference of 15000 movements -> progress bar: one '-' for each 20%)

Other changes
-------------
Suppression of Speed, heart rate and calorie.

How to build
------------
* Download [Code Composer Studio v5](http://processors.wiki.ti.com/index.php/Category:Code_Composer_Studio_v5).
* Import the project
* Build it
* Use [Flash programmer](http://www.ti.com/tool/flash-programmer) to send it to the watch