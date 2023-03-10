/**
 * @file   msxbusdrv.c
 * @author Miso Kim
 * @date   10 January 2022
 * @version 1,0
 * @brief   A MSX BUS driver to support the RPMC board 
 * Linux loadable kernel module (LKM) development. This module maps to /dev/msxbus and
 * comes with a helper C program that can be run in Linux user space to communicate with
 * this kernel module.
 * @see http://github.com/meesokim/blueberryMSX for a full description and follow-up descriptions.
 */
 
#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#include <linux/delay.h>
#include <linux/gpio.h>
#include "rpi-gpio.h"
#include "rpmcv8.h"

int msxread(char slot, unsigned short addr);
void msxwrite(char slot, unsigned short addr, unsigned char byte);
int msxreadio(unsigned short addr);
void msxwriteio(unsigned short addr, unsigned char byte);

#define	 CLASS_NAME "msx"
#define  DEVICE_NAME "msxbus"    ///< The device will appear at /dev/MSX Bus using this value
 
MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Miso Kim");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A MSX Bus driver for the RPMC board");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users
 
static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   msxdata[256*256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  msxbusClass  = NULL; ///< The device-driver class struct pointer
static struct device* msxbusDevice = NULL; ///< The device-driver device struct pointer
 
// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static long    dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

enum {
   SLOT1,
   SLOT3,
   IO
};

#define MSX_READ 0x10
#define MSX_WRITE 0

volatile unsigned int* gpio;
volatile unsigned int* gclk;


static int dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}
 
/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
 *  /linux/fs.h lists the callback functions that you wish to associated with your file operations
 *  using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .unlocked_ioctl = dev_ioctl,
   .write = dev_write,
   .release = dev_release,
};

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init msxbus_init(void){
   printk(KERN_INFO "MSX Bus: Initializing the MSX Bus LKM\n");
 
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "MSX Bus failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "MSX Bus: registered correctly with major number %d\n", majorNumber);
 
   // Register the device class
   msxbusClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(msxbusClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(msxbusClass);          // Correct way to return an error on a pointer
   }
   msxbusClass->dev_uevent = dev_uevent;
   printk(KERN_INFO "MSX Bus: device class registered correctly\n");
 
   // Register the device driver
   msxbusDevice = device_create(msxbusClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(msxbusDevice)){               // Clean up if there is an error
      class_destroy(msxbusClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(msxbusDevice);
   }
	gpio = (unsigned int *)ioremap_wc(GPIO_BASE, GPIO_GPPUDCLK1*4);
	gpio[GPIO_GPFSEL0] = GPIO_FSEL0_OUT | GPIO_FSEL1_OUT | GPIO_FSEL2_OUT | GPIO_FSEL3_OUT | GPIO_FSEL4_OUT | GPIO_FSEL5_OUT | GPIO_FSEL6_OUT | GPIO_FSEL7_OUT | GPIO_FSEL8_OUT | GPIO_FSEL9_OUT;
	gpio[GPIO_GPFSEL1] = GPIO_FSEL0_OUT | GPIO_FSEL1_OUT | GPIO_FSEL2_OUT | GPIO_FSEL3_OUT | GPIO_FSEL4_OUT | GPIO_FSEL5_OUT | GPIO_FSEL6_OUT | GPIO_FSEL7_OUT | GPIO_FSEL8_OUT | GPIO_FSEL9_OUT;
	gpio[GPIO_GPFSEL2] = GPIO_FSEL0_OUT | GPIO_FSEL1_OUT | GPIO_FSEL2_OUT | GPIO_FSEL3_OUT | GPIO_FSEL4_OUT | GPIO_FSEL5_OUT | GPIO_FSEL6_OUT | GPIO_FSEL7_OUT;   
   gpio[GPIO_GPPUD] = 2;
   ndelay(1500);
   gpio[GPIO_GPPUDCLK0] = MSX_WAIT;
   // ndelay(1500);
   // gpio[GPIO_GPPUD] = 0;
   // gpio[GPIO_GPPUDCLK0] = 0;
	GPIO_SET(LE_C | MSX_CONTROLS | MSX_WAIT | MSX_INT);
	GPIO_SET(LE_A | LE_D);
	GPIO_CLR(0xffff | MSX_RESET);
   mdelay(500);
	GPIO_SET(MSX_RESET);
   GPIO_SET(LE_C | LE_D | LE_A | 0xff00 | MSX_WAIT | MSX_RESET | MSX_INT);
   GPIO_CLR(LE_C);
   printk(KERN_INFO "MSX Bus: device class created correctly\n"); // Made it! device was initialized
   return 0;
}
 
/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit msxbus_exit(void){
   iounmap(gpio);
   device_destroy(msxbusClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(msxbusClass);                          // unregister the device class
   class_destroy(msxbusClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "MSX Bus: Goodbye!\n");
}
 
/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "MSX Bus: Device has been opened %d time(s)\n", numberOpens);
#if 1
	gclk = ioremap_wc(CLOCK_BASE, 0x80*4);
	if (gclk != NULL)
	{
		int divi, divr, divf, freq, divisor;
		int speed_id = 1;
		freq = 3500000;
		divi = 19200000 / freq ;
		divr = 19200000 % freq ;
		divf = (int)((double)divr * 4096.0 / 19200000.0) ;
		if (divi > 4095)
			divi = 4095 ;		
		divisor = 1 < 12;// | (int)(6648/1024);
		gclk[GP_CLK0_CTL] = 0x5A000000 | speed_id;    // GPCLK0 off
		while (gclk[GP_CLK0_CTL] & 0x80);    // Wait for BUSY low
		gclk[GP_CLK0_DIV] = 0x5A000000 | (divi << 12) | divf; // set DIVI
		gclk[GP_CLK0_CTL] = 0x5A000010 | speed_id;    // GPCLK0 on
		printk(KERN_INFO "clock enabled: 0x%08x\n", gclk[GP_CLK0_CTL] );
      iounmap(gclk);
	}
	else
		printk(KERN_INFO "clock disabled\n");   
#endif
   return 0;
}
 
/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   int count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   // printk(KERN_INFO "f_pos: %ll\n", *offset);
   while(len--)
   {
	   msxdata[count++] = msxread(0, *offset++);
   }
   error_count = copy_to_user(buffer, msxdata, count);
   if (error_count==0){            // if true then have success
      printk(KERN_INFO "MSX Bus: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "MSX Bus: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}
 
/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
//   sprintf(msxdata, "%s(%zu letters)", buffer, len);   // appending received string with its length
//   size_of_message = strlen(message);                 // store the length of the stored message
	int count = 0;
	while(len--)
	{
		msxwrite(0, filep->f_pos++, buffer[count++]);
	}
	len = count;
	printk(KERN_INFO "MSX Bus: Received %zu characters from the user\n", len);
   return len;
}
 
/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
	gclk = ioremap_wc(CLOCK_BASE, 0x80*4);
	if (gclk != NULL)
	{
      gclk[GP_CLK0_CTL] = 0x5A000000 | 1;    // GPCLK0 off
      iounmap(gclk);
   }   
   printk(KERN_INFO "MSX Bus: Device successfully closed\n");
   return 0;
}

static long dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {

   // local_irq_disable();
   long byte = 0;
   if (cmd & MSX_READ) {
      byte =  msxread(cmd & 0xf, arg & 0xffff);
   } else {
      msxwrite(cmd, arg & 0xffff, (arg >> 16) & 0xff);
   }
   // local_irq_enable();
   return byte;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(msxbus_init);
module_exit(msxbus_exit);

void SetAddress(unsigned short addr)
{
	
	GPIO_CLR(LE_C | 0xffff | DAT_DIR);
	GPIO_SET(LE_A | LE_D | addr);
	GPIO_SET(LE_A);
   GPIO_CLR(LE_A | LE_D);
	GPIO_SET(LE_C | MSX_CONTROLS);
	GPIO_CLR(LE_D | 0xff);

}	

void SetData(int ioflag, int flag, int delay, unsigned char byte)
{
	GPIO_SET(byte);
	GPIO_CLR(flag);
	GPIO_SET(MSX_MREQ | MSX_WR);
	GPIO_CLR(flag);
	while(!(GPIO_GET() & MSX_WAIT));
   GPIO_SET(MSX_CONTROLS);
	GPIO_CLR(LE_C);
	GPIO_CLR(LE_C);
}   

unsigned char GetData(int flag, int delay)
{
	unsigned char byte;
	GPIO_CLR(flag | 0xff);
	GPIO_SET(DAT_DIR);
	ndelay(delay);
	while(!(GPIO_GET() & MSX_WAIT));
   ndelay(10);
	byte = GPIO_GET();
  	GPIO_SET(MSX_CONTROLS);
   GPIO_SET(LE_D);
	return byte;
}

int msxread(char slot_io, unsigned short addr)
{
	unsigned char byte;
	int sio, cs1, cs2;
	cs1 = (addr & 0xc000) == 0x4000 ? MSX_CS1: 0;
	cs2 = (addr & 0xc000) == 0x8000 ? MSX_CS2: 0;
	SetAddress(addr);
   switch (slot_io) {
      case SLOT1:
         sio = MSX_SLTSL1 | MSX_MREQ | cs1 | cs2;
         break;
      case SLOT3:
         sio = MSX_SLTSL3 | MSX_MREQ | cs2 | cs2;
         break;
      case IO:
         sio = MSX_IORQ;
         break;
      default:
         return 0;      
   }
	byte = GetData(sio | MSX_RD, 250);
	return byte;	 
}
 
void msxwrite(char slot_io, unsigned short addr, unsigned char byte)
{
	int sio;
   switch (slot_io) {
      case SLOT1:
         sio = MSX_SLTSL1 | MSX_MREQ;
         break;
      case SLOT3:
         sio = MSX_SLTSL3 | MSX_MREQ;
         break;
      case IO:
         sio = MSX_IORQ;
      default:
         return;      
   }
	SetAddress(addr);
	SetData(MSX_MREQ, sio | MSX_WR, 90, byte);   
	return;
}
 
