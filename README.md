# char-dev-hex-output

---

# Linux Kernel Module: /dev/loop

## Overview

This Linux kernel module creates a character device driver `/dev/loop` that allows you to loop the input data into the `/tmp/output` file in a hexadecimal format with 16 bytes per row.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Getting Started](#getting-started)
- [How to Use](#how-to-use)
- [Authors](#authors)
- [License](#license)
- [Notes](#notes)

## Prerequisites

Before using this kernel module, you need:

- A Linux environment with kernel development tools and headers.
- Kernel module development experience.
- Appropriate permissions.

## Getting Started

To get started with this project:

1. Clone this repository:

   ```bash
   git clone https://github.com/DavidKuk/char-dev-hex-output.git
   ```

2. Build the kernel module:

   ```bash
   make
   ```

3. Load the kernel module:

   ```bash
   sudo insmod loop_cdev_module.ko
   ```

4. Verify the module is loaded:

   ```bash
   lsmod | grep loop_cdev_module
   ```

## How to Use

### Writing Data

To write data to the device, you can use standard file write operations. When you write data to the device, it will be processed and written to the `/tmp/output` file in a hexadecimal format with 16 bytes per row. The module converts data to hex format, including offset values.

Example: Currently only `root` user has permission to write to the `/dev/loop` characterr device file.

```bash
echo -e "Rainy days!" > /dev/loop
```

The content of `/tmp/output` file after writing to the `/dev/loop`
```txt
0000000 6152 6e69 2079 6164 7379 0a21
000000c
```

`hexdump` shows the same result as above
```bash
# echo -e "Rainy days!" | hexdump
0000000 6152 6e69 2079 6164 7379 0a21          
000000c
```

### Unloading the Module

To remove the module, run:

```bash
sudo rmmod loop_cdev_module
```

## Authors

- [David Kukulikyan](https://github.com/DavidKuk)

## License

This project is licensed under the [GNU General Public License (GPL)](LICENSE).

## Notes

- Code was tested and builded on `x86_64, 64-bit Little Endian, 5.15.0-84-generic` machine.
- For some file types writing in hexadecimal format to the file is not working properly.
