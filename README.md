# GPIO reset

gpio reset in linux kernel (using gpio file description)

## Getting Started

This reset test on FreeScale imx6, kernel version 4.1.15. Using board's gpio to control reset pin and control ISP mode of orther mcu (LCP11U68). 

### Prerequisites

To build this project, 
1. Appropriate arm compiler for your board
2. [Linux kernel for FreeScale](https://github.com/Freescale/linux-fslc)

This project use gpio port 6, pin 15 and pin 10
```
 imx6
pin 15 ----- reset
pin 10 ----- isp
```

### Installing

1. Modify device tree

Add this section to your device tree. Build and install in board
```
my_reset{
		compatible = "isp-reset";
		status = "okay";

		reset-gpios = <&gpio6 15 GPIO_ACTIVE_HIGH>;
		isp-gpios = <&gpio6 10 GPIO_ACTIVE_HIGH>;
	};
```

2. Fix *Makefile* kernel directory

```
...
KERNEL_SRC := /mydata/github/linux-fslc
...
```

3. Build module 
```
make CROSS_COMPILER=<your arm compiler>
```
or set in evironment variable
```
set CROSS_COMPILER=<your arm compiler>
make
```
After build complete, it will create file *reset_gpio.ko*.

4. Install as module in linux
Copy *reset_gpio.ko* to your arm board and run
```
insmod reset_gpio.ko
```

### Running

After install module into kernel, there will be 2 file can use in **/sys/class/reset_gpio/reset** and **/sys/class/reset_gpio/isp**
To reset MCU use
```
echo 1 > /sys/class/reset_gpio/reset
```

To enter ISP
```
echo 1 > /sys/class/reset_gpio/isp
```

To check status of ISP mode
```
cat /sys/class/reset_gpio/isp
```
if output is **isp mode** that mean you already set mcu in ISP mode at lease once, not grarentee that current status is in ISP mode (external reset mcu won't notify to host)

### License

This project is licensed under the GPLv2 License - see the LICENSE.md file for details
