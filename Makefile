# This Makefile is used to build the 'loop_cdev_module' kernel module
MODULE_VERSION := 0.2

# This line specifies the target object file (loop_cdev_module.o) for the kernel module
obj-m := loop_cdev_module.o

# This line retrieves the current kernel version using the 'uname' command and stores it in 'KERN_VERS'
KERN_VERS = $(shell uname -r)

# This section defines two Makefile targets: 'all' and 'clean'
all:
	# This line invokes the 'make' command in the kernel build directory to build the module
	# Please note that build succeeded on '5.11.0-40-generic' and '6.2.0-34-generic' kernel builds 
	make -C /lib/modules/$(KERN_VERS)/build M=$(PWD) modules
clean:
	# This line invokes the 'make' command in the kernel build directory to clean up the module build artifacts
	make -C /lib/modules/$(KERN_VERS)/build M=$(PWD) clean
