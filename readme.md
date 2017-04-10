	 / _____)             _              | |    
	( (____  _____ ____ _| |_ _____  ____| |__  
	 \____ \| ___ |    (_   _) ___ |/ ___)  _ \ 
	 _____) ) ____| | | || |_| ____( (___| | | |
	(______/|_____)_|_|_| \__)_____)\____)_| |_|
	  (C)2017 Semtech-Cycleo

LoRa Pico Cell Gateway project
==============================

1. Components
-------------

This directory contains the sources code of the Keil project to build a Picocell
Gateway MCU firmware based on the Semtech LoRa Picocell Gateway reference
design.
The target MCU is a STM32F401CD. The firmware implements an USB CDC protocol to
bridge commands coming from host to the SX1308 SPI interface.
The embedded firmware takes in charge the power management of the SX1308 during
the downlink to respect the 500MA max power constraint in the USB plug.


	((( Y )))
		|
		|
	+- -|- - - - - - - - - - - - -+        xxxxxxxxxxxx          +--------+
	|+--+-----------+     +------+|       xxx         xxx        |        |
	||              |     |      ||      xx  Internet   xx       |        |
	|| Pico Cell GW |<----+ Host |<-----xx     or       xx------>|        |
	|| STM32F401CD  | USB |      ||      xx  Intranet   xx       | Server |
	||   Sx1308     |     +------+|       xxx         xxx        |        |
	||  2*SX1257    |      Linux  |        xxxxxxxxxxxx          |        |
	|+--------------+             |                              |        |
	|                             |                              +--------+
	|                             |
	+- - - - - - - - - - - - - - -+


### 1.1. MAIN ###

The MAIN/ directory contains the main program that runs on the MCU, which
launches the USB command interpreters to communicate with the Host.

### 1.2. CmdUSB ###

The CmdUSB/ directory contains the USB command interpreter to handle the
communication between the Host and the concentrator. It is a bridge to the HAL
which communicates with the SX1308 through SPI.

### 1.3. usb_cdc ###

The STM32 USB CDC library modified for this reference design.

### 1.4. SX1308HAL ###

The LoRa concentrator Hardware Abstraction Layer C library to configure the
hardware, send and receive packets.

2. Precompiled binaries
-----------------------

The bin/ directory contains the precompiled binary files in .hex or .dfu
formats.
To load the binary file into the STM32F401CD target Mcu, use a tool like
DFU-UTIL : http://dfu-util.sourceforge.net/


3. Changelog
-------------

### v0.0.1  ###

* Initial release


4. Legal notice
----------------

The information presented in this project documentation does not form part of 
any quotation or contract, is believed to be accurate and reliable and may be 
changed without notice. No liability will be accepted by the publisher for any 
consequence of its use. Publication thereof does not convey nor imply any 
license under patent or other industrial or intellectual property rights. 
Semtech assumes no responsibility or liability whatsoever for any failure or 
unexpected operation resulting from misuse, neglect improper installation, 
repair or improper handling or unusual physical or electrical stress 
including, but not limited to, exposure to parameters beyond the specified 
maximum ratings or operation outside the specified range. 

SEMTECH PRODUCTS ARE NOT DESIGNED, INTENDED, AUTHORIZED OR WARRANTED TO BE 
SUITABLE FOR USE IN LIFE-SUPPORT APPLICATIONS, DEVICES OR SYSTEMS OR OTHER 
CRITICAL APPLICATIONS. INCLUSION OF SEMTECH PRODUCTS IN SUCH APPLICATIONS IS 
UNDERSTOOD TO BE UNDERTAKEN SOLELY AT THE CUSTOMER'S OWN RISK. Should a
customer purchase or use Semtech products for any such unauthorized 
application, the customer shall indemnify and hold Semtech and its officers, 
employees, subsidiaries, affiliates, and distributors harmless against all 
claims, costs damages and attorney fees which could arise.

*EOF*
