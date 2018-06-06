# Heterogeneous Embedded Architectures Programming Library -- stm32f4 target

For the main project see https://github.com/gkirgizov/hetarch.
It is a target runtime for stm32f4i-discovery microcontoller.

Code is based on project template generated with STM32CUBE framework.
Makefile is mostly generated too, only adjusted at places (e.g. to include needed files and enable debugging with openocd).

Makefile targets must be more or less self-explanatory:
* make -- builds the project
* make program -- programs connected microcontroller
* make openocd -- starts openocd server for debugging
* make debug -- debug with openocd
