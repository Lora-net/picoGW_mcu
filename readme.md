	 / _____)             _              | |    
	( (____  _____ ____ _| |_ _____  ____| |__  
	 \____ \| ___ |    (_   _) ___ |/ ___)  _ \ 
	 _____) ) ____| | | || |_| ____( (___| | | |
	(______/|_____)_|_|_| \__)_____)\____)_| |_|
	  (C)2017 Semtech-Cycleo

LoRa Pico Cell Gateway project - MCU firmware
=============================================


## 1. Components
----------------

This directory contains the sources code of the Keil project to build a Picocell
Gateway MCU firmware based on the Semtech LoRa Picocell Gateway reference
design.
The target MCU is a STM32F401CD. The firmware implements either a USB CDC
protocol or a UART protocol to bridge commands coming from host to the SX1308
SPI interface.
The embedded firmware takes in charge the power management of the SX1308 during
the downlink to respect the 500MA max power constraint in the USB plug.


	((( Y )))
		|
		|
	+- -|- - - - - - - - - - - - -+        xxxxxxxxxxxx          +--------+
	|+--+-----------+     +------+|       xxx         xxx        |        |
	||              |     |      ||      xx  Internet   xx       |        |
	|| Pico Cell GW |<----+ Host |<-----xx     or       xx------>|        |
	|| STM32F401CD  | USB/|      ||      xx  Intranet   xx       | Server |
	||   Sx1308     | UART+------+|       xxx         xxx        |        |
	||  2*SX1257    |      Linux  |        xxxxxxxxxxxx          |        |
	|+--------------+             |                              |        |
	|                             |                              +--------+
	|                             |
	+- - - - - - - - - - - - - - -+


### 1.1. MAIN ###

The MAIN/ directory contains the main program that runs on the MCU, which
launches the USB/UART commands interpreter to communicate with the Host.

### 1.2. CmdUSB ###

The CmdUSB/ directory contains the USB/UART command interpreter to handle the
communication between the Host and the concentrator. It is a bridge to the HAL
which communicates with the SX1308 through SPI.

### 1.3. usb_cdc ###

The STM32 USB CDC library modified for this reference design.

### 1.4. SX1308HAL ###

The LoRa concentrator Hardware Abstraction Layer C library to configure the
hardware, send and receive packets.


## 2. Precompiled binaries
--------------------------

The bin/ directory contains the precompiled binary files in .hex or .dfu
formats, for both USB and UART communication bridges.
To load the binary file into the target MCU flash memory, you have to:

### 2.1. USB

* Make the MCU enter DFU mode:
    - either press the BOOT0 button of the PicoCell GW while plugging it to the
    host USB port
    - or use the util_boot tool provided with the picoGW_hal repository.

* Program the binary into the MCU flash memory:
    - use a tool like dfu-util (http://dfu-util.sourceforge.net).

    ex:
    ```
    dfu-util -a 0 -D pgw_fw_usb.dfu
    ```

    Note: you may have to use sudo depending on your priviledges.

* Unplug/replug USB PicoCell GW to exit DFU mode.


### 2.2. UART

* Make the MCU enter UART bootloader:
    - either press the BOOT0 button of the PicoCell GW while plugging it to the
    host USB port
    - or use the util_boot tool provided with the picoGW_hal repository.

* Program the binary into the MCU flash memory:
    - use a tool like stm32flash (https://sourceforge.net/projects/stm32flash)

    ex:
    ```
    stm32flash -b 115200 /dev/ttyS0 -v -w pgw_fw_uart.hex -g 0x0
    ```


## 3. Build the project
-----------------------

In order to compile the whole project, a Keil project is provided in the src/
directory. Open the pgw.uvproj file in Keil and rebuild all.

By default, it builds the project for using USB communication bridge. In order
to use the UART instead, uncomment the following line in src/MAIN/board.h before
building all the project.

```
#define USE_UART 1
```

After the compilation as succeeded, a Pgw.hex file is created in the bin/
directory. It is the file to be programmed in the MCU flash memory. Refer to
section 2 of this document to get information for flashing.
A DFU file can be created from this .hex file using the "Dfu file manager" tool
provided by STmicroelectronics.
[www.st.com/resource/en/user_manual/cd00155676.pdf](www.st.com/resource/en/user_manual/cd00155676.pdf)


## 4. User Guide
----------------

[A detailed PicoCell GW user guide is available here](http://www.semtech.com/images/datasheet/picocell_gateway_user_guide.pdf)


## 5. Changelog
---------------

### v0.2.0  ###

* Reverted AGC firmware from version 5 back to version 4.
* Updated MCU FW version to match with picoGW_hal

### v0.1.1  ###

* Disabled USB vbus sensing
* Moved delay to wait for COM bridge to be initialized in CMDMANAGER::Init().

### v0.1.0  ###

* Code clean-up, refactoring
* Support both USB and UART communication with host

### v0.0.1  ###

* Initial release


## 6. Legal notice
------------------

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
