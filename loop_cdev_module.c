#include <linux/kernel.h>  /* Provides access to various kernel-related functions and macros that are useful when developing kernel modules */
#include <linux/init.h>    /* Containes macros and functions related to module initialization and cleanup */
#include <linux/module.h>  /* Containes necessary macros, functions, and data structures for defining and implementing kernel modules */
#include <linux/fs.h>      /* For working with the file systems and file-related operations */
#include <linux/err.h>     /* For working with error handling and error code definitions */
#include <linux/cdev.h>    /* For working with character devices and character device drivers */
#include <linux/slab.h>    /* For working with kmalloc and kfree */

#define FILE_PATH "/tmp/output"
#define DEV_NAME "loop"
#define FAILURE -1
#define SUCCESS  1
#define F_OPEN_TRUNCATE_MODE  0
#define F_OPEN_APPEND_MODE  1

dev_t dev = 0;
static struct class *dev_class;
static struct cdev loop_cdev;
loff_t F_OFFSET;

static int      __init loop_driver_init(void);
static void     __exit loop_driver_exit(void);
static int      dev_open(struct inode *inode, struct file *file);
static int      dev_release(struct inode *inode, struct file *file);
static ssize_t  dev_write(struct file *filp, const char *buf, size_t len, loff_t *offset);
struct file     *open_file_in_write_mode(short open_mode);
int             write_to_the_file(struct file *file, char *buffer, const size_t size, loff_t offset);
static void     write_spaces(struct file *file, uint16_t max_row_bytes, uint16_t row_bytes, loff_t offset);
static void     write_to_the_file_in_hex_format(char *buffer, size_t len, loff_t offset);

/*
 * Structure that defines file operations (write, open, release)
 */
static struct file_operations fops =
{
    .owner      = THIS_MODULE,
    .write      = dev_write,
    .open       = dev_open,
    .release    = dev_release
};

/*
 * The function just prints info to the kernel log file when device driver is opened
 */
static int dev_open(struct inode *inode, struct file *file) {
        pr_info("Device driver is opened...");
        return 0;
}

/*
 * dev_release - Function is called when the device driver is released or closed. 
 * Responsible to writing the last OFFSET to the end of the file (/tmp/output) in hex format.
 *
 * @param inode - Pointer to the inode structure.
 * @param file - Pointer to the file structure.
 *
 * Function returns 0 on success.
 * 
 */
static int dev_release(struct inode *inode, struct file *file) {
    struct file *out_file;
    char offset_in_hex [8];
    loff_t offset = F_OFFSET;
    if((out_file = open_file_in_write_mode(F_OPEN_APPEND_MODE)) == NULL){
        pr_err("Couldn't open the '%s' file", FILE_PATH);
        filp_close(out_file, NULL);
    } else {
        pr_err("Writing the OFFSET to the end of '%s' file", FILE_PATH);
        write_to_the_file(out_file, "\n", 1, offset);
        snprintf(offset_in_hex, sizeof(offset_in_hex), "%07llx", offset);
        write_to_the_file(out_file, offset_in_hex, sizeof(offset_in_hex)-1, offset);
        write_to_the_file(out_file, "\n", 1, offset);
    }
    pr_info("Device driver is released...");
    return 0;
}


/**
 * open_file_in_write_mode - Open a file in write mode with specified options.
 *
 * @param open_mode - The mode for opening the file (F_OPEN_TRUNCATE_MODE or F_OPEN_APPEND_MODE).
 *
 * This function opens a file at the specified path in write mode with options based on
 * the specified open_mode. It can either create and truncate the file or append to it.
 *
 * This function returns a pointer to the opened file structure, or NULL if there was an error.
 */
struct file *open_file_in_write_mode(short open_mode) {
    struct file *file;
    if (open_mode == F_OPEN_TRUNCATE_MODE){
        file = filp_open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0664);
    } else if (open_mode == F_OPEN_APPEND_MODE) {
        file = filp_open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0664);
    } else
    {
      pr_err("Incorrect open mode was specified!");
      return NULL;
    }
    if (IS_ERR(file)) {
        pr_err("Error while opening the '%s' file.", FILE_PATH);
        return NULL;
    } else {
        pr_info("The '%s' file successfully opened", FILE_PATH);
        return file;
    }
}

/**
 * write_to_the_file - Write data to a file at a specified offset.
 *
 * @file: File structure representing the target file.
 * @buffer: Pointer to the data buffer to be written.
 * @size: Size of the data to be written.
 * @offset: Offset within the file where the data should be written.
 *
 * This function writes the data from the buffer to the specified file at the given offset.
 * It checks for errors during the file opening and writing process and returns success (1)
 * or failure (-1) accordingly.
 */
int write_to_the_file(struct file *file, char *buffer, const size_t size, loff_t offset) {
    size_t bytes_written;
    if (IS_ERR(file)) {
        pr_err("Error while opening the file");
        return FAILURE;
    }
    bytes_written = kernel_write(file, buffer, size, &offset);
    if (bytes_written < 0) {
        pr_err("Error while writing bytes!!!\n");
        return FAILURE;
    }
    return SUCCESS;
}

/**
 * write_spaces - Writes spaces in the file (/tmp/output) to pad row with fewer than 16 bytes.
 *
 * @filp: File structure representing the file to write to.
 * @max_row_bytes: Maximum bytes for a row
 * @row_bytes: Actual written bytes for a row
 * @off: File offset
 *
 */
static void write_spaces(struct file *file, uint16_t max_row_bytes, uint16_t row_bytes, loff_t offset){
    short space_cnt;
    char *spaces = NULL;
    space_cnt = (max_row_bytes - (row_bytes)) * 2 + 8 - (row_bytes/2);
    pr_info("Space count: %hu", space_cnt);
    spaces = kmalloc(space_cnt, GFP_KERNEL); // Allocate memory from kernel space
    if (!spaces) {
        pr_err("Failed to allocate memory from kernel space... Function name: %s", __func__);
    } else {
        memset(spaces, ' ', space_cnt);
        write_to_the_file(file, spaces, space_cnt, offset);
        kfree(spaces);
    }
}

/**
 * write_to_the_file_in_hex_format - Convert and write data in hexadecimal format to a file
 *
 * @buffer: Pointer to the input character buffer.
 * @len: Length of the input data.
 * @off: File offset
 *
 * This function takes a character buffer, converts its contents into hexadecimal format,
 * and writes the data to a file. It ensures that the output is formatted with 16 bytes per row,
 * and includes hexadecimal offset values at the beginning of each row. The data is written to
 * the file in a formatted manner.
 *
 */
static void write_to_the_file_in_hex_format(char *buffer, size_t len, loff_t offset){
    struct file *file;
    char *ptr = buffer;
    uint16_t max_row_bytes = 16, row_bytes = 0;
    char offset_in_hex [8], bytes_in_hex [6];
    short f_open_mode = F_OPEN_TRUNCATE_MODE;
    if (offset > 0) {
            f_open_mode = F_OPEN_APPEND_MODE;
    }
    if((file = open_file_in_write_mode(f_open_mode)) == NULL){
        pr_err("Couldn't open the '%s' file", FILE_PATH);
        filp_close(file, NULL);
    } else {
        pr_info("Writing buffer content to the '%s' file in hex format (For each row 16 byte)...", FILE_PATH);
        while(ptr < buffer + len){
            if ((ptr + 1) < buffer + len){
                unsigned short two_bytes = (((unsigned short)ptr[1]) << 8) | (uint8_t)ptr[0];
                if ((row_bytes % max_row_bytes) == 0){
                    offset+=row_bytes;
                    if (offset > 0){
                        write_to_the_file(file, "\n", 1, offset);
                    }
                    snprintf(offset_in_hex, 8, "%07llx", offset);
                    write_to_the_file(file, offset_in_hex, sizeof(offset_in_hex)-1, offset);
                    row_bytes = 0;
                }
                snprintf(bytes_in_hex, 6, " %04x", two_bytes);
                write_to_the_file(file, bytes_in_hex, sizeof(bytes_in_hex)-1, offset);
            }
            ptr+=2;
            row_bytes+=2;
        }
        if((len % 2) != 0){
            unsigned short two_bytes = (unsigned short) buffer[len-1];
            snprintf(bytes_in_hex, 6, " %04x", two_bytes);
            write_to_the_file(file, bytes_in_hex, sizeof(bytes_in_hex)-1, offset);
        }
        offset+=row_bytes;
        if ((max_row_bytes - row_bytes) != 0 ){
            write_spaces(file, max_row_bytes, row_bytes, offset);
        }
        filp_close(file, NULL);
        pr_info("Writing buffer content to the file in hex format is completed.");
    }
}

/**
 * dev_write - Write data to the device driver
 *
 * @filp:      File structure representing the file to write to.
 * @buf:       User-space buffer containing data to be written.
 * @len:       Length of the data to be written.
 * @offset:       Pointer to the current file offnset.
 *
 * This function allocates kernel memory, copies data from the user space, writes
 * the data to a file in hexadecimal format, and then releases the allocated memory.
 *
 * Returns the number of bytes written (len).
 */
static ssize_t dev_write(struct file *filp, const char __user *buf, size_t len, loff_t *offset) {
        char *kernel_buffer = kmalloc(len, GFP_KERNEL); // Allocate memory from kernel space
        if (!kernel_buffer) {
            pr_err("Failed to allocate memory from kernel space... Function name: %s", __func__);
        } else {
            int bytes_copied = copy_from_user(kernel_buffer, buf, len);
            if (bytes_copied == 0) {
                pr_info("Bytes are copied to kernel space from user space...");
            }
            write_to_the_file_in_hex_format(kernel_buffer, len, *offset);
            *offset += len;
            kfree(kernel_buffer);
        }
        F_OFFSET = *offset;
        return len;
}

/**
 * loop_driver_init - Initialize the character device driver
 *
 * This function initializes the character device driver by allocating a major number,
 * creating a character device structure, adding it to the system, creating a struct class,
 * and creating a loop device.
 *
 * Returns 0 on success and -1 on failure.
 */
static int __init loop_driver_init(void) {
        /*Allocating Major number*/
        if((alloc_chrdev_region(&dev, 0, 1, "loop_cdev")) <0){
                pr_err("Cannot allocate major number...");
                return FAILURE;
        }
        pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));
        /*Creating cdev structure*/
        cdev_init(&loop_cdev,&fops);
        /* Adding character device to the system */
        if((cdev_add(&loop_cdev, dev, 1)) < 0){
            pr_err("Can not add the device to the system...");
            goto r_class;
        }
        /*Creating struct class*/
        if(IS_ERR(dev_class = class_create(THIS_MODULE, "loop_cdev_class"))){
            pr_err("Cannot create the struct class");
            goto r_class;
        }
        /*Creating loop device*/
        if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "loop"))){
            pr_err("Cannot create the %s Device", DEV_NAME);
            goto r_device;
        }
        pr_info("Device driver successfully inserted...");
      return 0;
r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev, 1);
        return FAILURE;
}

/**
 * loop_driver_exit - Clean up and remove the character device driver
 *
 * This function performs cleanup tasks, including removing the loop device, destroying
 * the device class, deleting the character device structure, and unregistering the
 * allocated major number. It is called when the module is unloaded or removed.
 */
static void __exit loop_driver_exit(void) {
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&loop_cdev);
        unregister_chrdev_region(dev, 1);
        pr_info("Device driver successfully removed...");

}

MODULE_LICENSE("GPL"); // Specifies the license under which the module is released (GNU General Public License).
MODULE_AUTHOR("David Kukulikyan <davidkuk25@gmail.com>"); // Specifies the author's name and contact email.
// Provides a description of the module's functionality
MODULE_DESCRIPTION("Linux kernel device driver that creates /dev/loop device that loops the input into /tmp/output file with in a hex format (16 bytes per row).");
MODULE_VERSION("0.4"); // Specifies the version of the module (0.4).

// These lines define which functions should be called when the module is loaded and unloaded.
module_init(loop_driver_init);
module_exit(loop_driver_exit);
