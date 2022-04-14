Micro SAM-BA
------------

# Description

This simple tool can be used to program the internal flash of the following
microcontrollers:

  - Atmel SAME70/S70/V70/V71

Communication with SAM-BA bootloader only supported through USB and UART (UART untested).

Before using the tool, the MCU must be put in "Boot from ROM" mode, either by
fully erasing it using the ERASE pin or by clearing GPVNM1 (see datasheet for
more information).  When the device is in this mode, it will be enumerated as:

- 03eb:6124 Atmel Corp. at91sam SAMBA bootloader
    
# Getting started
`` cc -std=gnu99 -Wall -O2 -Wfatal-errors -I./src   -c -o src/usamba.o src/usamba.c``
`` cc   src/usamba.o src/comm.o src/chipid.o src/eefc.o   -o src/usamba``

# Usage

Usage: ``./usamba <port> (read|write|verify|erase-all|erase-pages|ext-read|ext-write|ext-erase-all|gpnvm|identify|reset|exit-samba) [args]*``


- **Read Flash**:
    ``./usamba <port> read <filename> <start-address> <size>``
       Note: you may forgo the filename and the program will automatically make its own filename with the Unique ID & Serial Number
       **Ex**:
      ``` ./usamba /dev/ttyACM1 read 0x00020000 32 ```
      ```./usamba /dev/ttyACM1 read image.bin 0x00020000```
       
- **Write Flash**:
    ``./usamba <port> write <filename> <start-address>``
    **Ex**:``` ./usamba /dev/ttyACM1 write image.bin 0x00000000 ```

- **Verify Flash**:
    ``./usamba <port> verify <filename> <start-address>``
    **Ex**: ``` ./usamba /dev/ttyACM1 verify imagine.bin 0x00000000 ```

- **Erase Flash**:
    ``./usamba <port> erase-all``
    **Ex**: ``` ./usamba /dev/ttyACM1 erase-all```    

- **Erase 16 Pages**:
    ``./usamba <port> erase-pages <first-page>``
    **Ex**: ``` ./usamba /dev/ttyACM1 erase-pages 0 ```

- **Reading External Flash**:
    ``./usamba <port> ext-read <filename> <start-address> <size>``
    **Ex**:``` ./usamba /dev/ttyACM1 ext-read image.bin 0x00020000 32 ```

- **Writing External Flash**:
    ``./usamba <port> ext-write <filename> <start-address>``
    **Ex**: ``` ./usamba /dev/ttyACM1 ext-write image.bin 0x00020000 32 ```

- **Erasing External Flash**:
    ``./usamba <port> ext-erase-all``
    **Ex**: ``` ./usamba /dev/ttyACM1 ext-erase-all```

- **Get/Set/Clear GPNVM**:
    ``./usamba <port> gpnvm (get|set|clear) <gpnvm_number>``
    **Ex**: ```./usamba /dev/ttyACM1 gpnvm clear 0```

- **Identify Chip's unique identifier code & serial number**:
    ``./usamba <port> identify``
    **Ex**: ```./usamba /dev/ttyACM1 identify```

- **Reset device**:
    ``./usamba <port> reset``
    **Ex**: ```./usamba /dev/ttyACM1 reset```    
- **Exit Samba**:
    ``./usamba <port> exit-samba``
    **Ex**: ```./usamba /dev/ttyACM1 exit-samba```

### For all commands
``<port>`` is the USB device node for the SAM-BA bootloader, for example ``/dev/ttyACM0``.
``<start-address>`` and ``<size>`` can be specified in decimal, hexadecimal (if prefixed by ``0x``) or octal (if prefixed by ``0``).
