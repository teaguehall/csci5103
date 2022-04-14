/*
 * main.c -- the bare scull char module
 *
 * This code is based on the scullpipe code from LDD book.
 *         Anand Tripathi March 2022
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */
 
#include <linux/sched.h>

#include <linux/configfs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <linux/uaccess.h>	/* copy_*_user */
#include <linux/sched/signal.h>
#include "scullbuffer.h"	/* local definitions */

//#define DEBUG_PRINT

/*
 * Our parameters which can be set at load time.
 */

struct scull_buffer {
        wait_queue_head_t inq, outq;   // read and write queues
        char *buffer, *end;            // begin of buf, end of buf
        int buffersize;                // used in pointer arithmetic
        char *rp, *wp;                 // where to read, where to write
        int  itemcount;      		   // Number of items in the buffer
        int nreaders, nwriters;        // number of openings for r/w
        struct semaphore sem;          // mutual exclusion semaphore
        struct cdev cdev;              // Char device structure
};

/* parameters */
static int scull_b_nr_devs = SCULL_B_NR_DEVS;	// number of buffer devices
dev_t scull_b_devno;							// our first device number

static struct scull_buffer *scull_b_devices;
static char *scull_b_buffers;

#define init_MUTEX(_m) sema_init(_m, 1);

int scull_major =   SCULL_MAJOR;
int scull_minor =   0;
int NITEMS	=  20;
int itemsize = SCULL_B_ITEM_SIZE;

module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);
module_param(scull_b_nr_devs, int, 0);
module_param(NITEMS, int, 0);

MODULE_AUTHOR("Student CSCI 5103-S2022 - adding code to the framework");
MODULE_LICENSE("Dual BSD/GPL");

/*
 * Open and close
 */
static int scull_b_open(struct inode *inode, struct file *filp)
{
	struct scull_buffer *dev;
	
	dev = container_of(inode->i_cdev, struct scull_buffer, cdev);
	filp->private_data = dev; /* for other methods */
	
	// grab mutex
	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	
	if((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		dev->nwriters++;
		
		#ifdef DEBUG_PRINT 
		printk(KERN_ALERT "Writer opened. Writer count = %d\n", dev->nwriters);
		#endif
	}
	else if((filp->f_flags & O_ACCMODE) == O_RDONLY)
	{
		dev->nreaders++;
		
		#ifdef DEBUG_PRINT 
		printk(KERN_ALERT "Reader opened. Reader count = %d\n", dev->nreaders);
		#endif
	}
	
	// release mutex
	up(&dev->sem);
	
	return nonseekable_open(inode, filp);
}

static int scull_b_release(struct inode *inode, struct file *filp)
{
	struct scull_buffer *dev; //  = filp->private_data;

	dev = container_of(inode->i_cdev, struct scull_buffer, cdev);
	filp->private_data = dev; /* for other methods */
	
	// grab mutex
	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	
	if((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		dev->nwriters--;
		
		#ifdef DEBUG_PRINT 
		printk(KERN_ALERT "Writer released. Writer count = %d\n", dev->nwriters);
		#endif
	}
	else if((filp->f_flags & O_ACCMODE) == O_RDONLY)
	{
		dev->nreaders--;
		
		#ifdef DEBUG_PRINT 
		printk(KERN_ALERT "Reader released. Reader count = %d\n", dev->nreaders);
		#endif
	}
	
	// release mutex
	up(&dev->sem);
	
	// TODO - actually implement this.
	// IMPLEMENT THIS FUNCTION 
	//
	
	return 0;
}

// write handler
static ssize_t scull_b_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_buffer *dev = filp->private_data;
	size_t clamped_count;
	
	// grab mutex
	if(down_interruptible(&dev->sem))
		return -1;
	
	// wait until slot opens up 
	while(dev->itemcount >= dev->buffersize)
	{
		// release mutex and exit if no active readers
		if(!dev->nreaders)
		{
			up(&dev->sem);
			return 0;
		}
		
		// otherwise release mutex and go to sleep
		up(&dev->sem);
		if(wait_event_interruptible(dev->inq, dev->itemcount < dev->buffersize))
			return -1;
		
		// acquire mutex again
		if(down_interruptible(&dev->sem))
			return -1;
	}

	// clamp byte count (to make sure it fits within slot size)
	if(count > SCULL_B_ITEM_SIZE)
	{
		clamped_count = SCULL_B_ITEM_SIZE;
	}
	else
	{
		clamped_count = count;
	}
	
	// copy item to buffer, increment item count, and send wake up signal
	copy_from_user(dev->wp, buf, clamped_count);
	dev->itemcount++;
	wake_up_interruptible(&dev->outq);
	
	#ifdef DEBUG_PRINT 
	printk(KERN_ALERT "Added item. Item count = %d\n", dev->itemcount);
	#endif
	
	// increment write pointer (roll-over if necessary).
	dev->wp += SCULL_B_ITEM_SIZE;
	if(dev->wp >= dev->end)
	{
		dev->wp = dev->buffer;
	}
	
	// success (release mutex and return number of bytes written)
	up(&dev->sem);
	return clamped_count;
	
}

// read handler
static ssize_t scull_b_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_buffer *dev = filp->private_data;
	
	// grab mutex
	if(down_interruptible(&dev->sem))
		return -1;
	
	// wait until item to consume
	while(dev->itemcount == 0)
	{
		// release mutex and exit if no active writers
		if(!dev->nwriters)
		{
			up(&dev->sem);
			return 0;
		}
		
		// otherwise release mutex and go to sleep
		up(&dev->sem);
		if(wait_event_interruptible(dev->outq, dev->itemcount > 0))
			return -1;
		
		// acquire mutex again
		if(down_interruptible(&dev->sem))
			return -1;
	}
	
	// copy item from buffer and decrement item count
	copy_to_user(buf, dev->rp, SCULL_B_ITEM_SIZE);
	dev->itemcount--;
	wake_up_interruptible(&dev->inq);
	
	#ifdef DEBUG_PRINT 
	printk(KERN_ALERT "Removed item. Item count = %d\n", dev->itemcount);
	#endif
	
	// increment read pointer (roll-over if necessary).
	dev->rp += SCULL_B_ITEM_SIZE;
	if(dev->rp >= dev->end)
	{
		dev->rp = dev->buffer;
	}
	
	// success (release mutex and return number of bytes written)
	up(&dev->sem);
	return SCULL_B_ITEM_SIZE;
} 

/*
 * The file operations for the buffer device
 * This is S2022 framework codebase.
 */
struct file_operations scull_buffer_fops = {
	.owner =	THIS_MODULE,
	.llseek =	no_llseek,
	.read =		scull_b_read,
	.write =	scull_b_write,
	.open =		scull_b_open,
	.release =	scull_b_release,
};

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void scull_b_cleanup_module(void)
{
	int i;
	dev_t devno = MKDEV(scull_major, scull_minor);
	
	#ifdef DEBUG_PRINT 
	printk(KERN_ALERT "Unloading %u scull buffers\n", scull_b_nr_devs);
	#endif
	
	// free scull devices
	if (scull_b_devices) 
	{
		for (i = 0; i < scull_b_nr_devs; i++) {
			cdev_del(&scull_b_devices[i].cdev);
		}
		
		kfree(scull_b_devices);
	}
	
	// free buffers\n
	if(scull_b_buffers)
	{
		kfree(scull_b_buffers);
	}
	
	// unregister device
	unregister_chrdev_region(devno, scull_b_nr_devs);
}

// set up a cdev entry.
static void scull_b_setup_cdev(struct scull_buffer *dev, int index)
{
	int err, devno = MKDEV(scull_major, scull_minor + index);
    
	cdev_init(&dev->cdev, &scull_buffer_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_buffer_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	// fail gracefully if need be
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

int scull_b_init_module(void)
{
	int i, result;
	dev_t devno;
	
	#ifdef DEBUG_PRINT 
	printk(KERN_ALERT "Loading %u scull buffers\n", scull_b_nr_devs);
	#endif
	
	// if major number specified, try to register it
	if (scull_major)
	{
		devno = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(devno, scull_b_nr_devs, "scullbuffer");
	}
	else // otherwise dynamically register major number
	{
		result = alloc_chrdev_region(&devno, scull_minor, scull_b_nr_devs, "scullbuffer");
		scull_major = MAJOR(devno);
	}
	if (result < 0)  // exit with error
	{
		printk(KERN_WARNING "scullbuffer: can't get major %d\n", scull_major);
		return result;
	}
	
	// allocate scull buffer devices
	scull_b_devices = kmalloc(scull_b_nr_devs * sizeof(struct scull_buffer), GFP_KERNEL);
	if (!scull_b_devices)
	{
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(scull_b_devices, 0, scull_b_nr_devs * sizeof(struct scull_buffer));
	
	// allocate buffer space for each device
	scull_b_buffers = kmalloc(scull_b_nr_devs * NITEMS * SCULL_B_ITEM_SIZE, GFP_KERNEL);
	if (!scull_b_buffers)
	{
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(scull_b_buffers, 0, scull_b_nr_devs * NITEMS * SCULL_B_ITEM_SIZE);
		
	// initialize each device
	for (i = 0; i < scull_b_nr_devs; i++) {
		init_waitqueue_head(&(scull_b_devices[i].inq));
		init_waitqueue_head(&(scull_b_devices[i].outq));
		scull_b_devices[i].buffer = scull_b_buffers + i * NITEMS * SCULL_B_ITEM_SIZE;
		scull_b_devices[i].end = scull_b_devices[i].buffer + NITEMS * SCULL_B_ITEM_SIZE;
		scull_b_devices[i].buffersize = NITEMS;
		scull_b_devices[i].rp = scull_b_devices[i].buffer;
		scull_b_devices[i].wp = scull_b_devices[i].buffer;
		scull_b_devices[i].itemcount = 0;
		scull_b_devices[i].nreaders = 0;
		scull_b_devices[i].nwriters = 0;
		init_MUTEX(&scull_b_devices[i].sem);
		scull_b_setup_cdev(&scull_b_devices[i], i);
	}
	
	// success
	return 0;
	
	// clean-up code if failure occurs during module loading
	fail:
		scull_b_cleanup_module();
		return result;	
}

module_init(scull_b_init_module);
module_exit(scull_b_cleanup_module);
