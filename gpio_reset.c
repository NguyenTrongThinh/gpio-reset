#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>       // for fs function like alloc_chrdev_region / operation file
#include <linux/types.h>
#include <linux/device.h>   // for device_create and class_create
#include <linux/cdev.h>     // cdev operate function
#include <linux/uaccess.h>  // for copy to/from user function
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of.h>       // access device tree file
#include <linux/delay.h>
#include <linux/slab.h>     // kmalloc, kcallloc, ....
#include <linux/mutex.h>

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("THOMASTHONG");
MODULE_VERSION("1.0.0");

#define PDEBUG(fmt,args...) printk(KERN_DEBUG"%s: "fmt,DRIVER_NAME, ##args)
#define PERR(fmt,args...) printk(KERN_ERR"%s: "fmt,DRIVER_NAME,##args)
#define PINFO(fmt,args...) printk(KERN_INFO"%s: "fmt,DRIVER_NAME, ##args)

#define DRIVER_NAME "reset_gpio"
#define FIRST_MINOR 0
#define BUFF_SIZE 100

dev_t device_num ;
struct class * device_class;
struct device * device;
struct gpio_desc *reset, *isp;
typedef struct privatedata {
    bool isp_mode ;

	struct mutex lock;
} private_data_t;
private_data_t *data;

static int driver_probe (struct platform_device *pdev);
static int driver_remove(struct platform_device *pdev);

static const struct of_device_id reset_dst[]={
    //{ .compatible = "acme,foo", },
    { .compatible = "isp-reset", },
    {}
};

MODULE_DEVICE_TABLE(of, reset_dst);	

static struct platform_driver reset_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,   
        .of_match_table = of_match_ptr (reset_dst),
    },
    .probe = driver_probe,
    .remove = driver_remove,
};


/***********************************/
/***** define device attribute *****/
/***********************************/
// define attribute is isp and reset

static ssize_t isp_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t len)
{
    private_data_t *data = dev_get_drvdata(dev);
    if (!data)
        PERR("Can't get private data from device, pointer value: %d\n", data);
    PINFO ("inside isp_store, store %d byte, first byte value: %c\n", len, buff[0]);

    if (buff[0] == '1' && (len == 2))
    {
       mutex_lock(&data->lock);
       gpiod_set_value(isp,0);
       gpiod_set_value(reset, 1); 

       msleep(100);

       gpiod_set_value(reset, 0);
       gpiod_set_value(isp, 1);
       data->isp_mode = true;
       mutex_unlock(&data->lock);
    } 

    return len;
}

static ssize_t isp_show(struct device *dev, struct device_attribute *attr, char *buf)
{ 
    int res;
    private_data_t *data = dev_get_drvdata(dev);
    if (!data)
        PERR("Can't get private data from device, pointer value: %d\n", data);

    res = scnprintf(buf, PAGE_SIZE, "%s\n", data->isp_mode ? "isp mode" : "normal mode");

    return res;
}

// create struct device_attribute variable
static DEVICE_ATTR_RW(isp);

static ssize_t reset_store(struct device *dev, struct device_attribute *attr, const char *buff, size_t len)
{
    int res;
    private_data_t *data = dev_get_drvdata(dev);
    if (data)
        PERR("Can't get private data from device, pointer value: %d\n", data);
    PINFO ("inside reset_store %d byte, first byte value: %c\n", len, buff[0]);

    if (buff[0] == '1' && (len == 2))
    {
       mutex_lock(&data->lock);
       gpiod_set_value(reset, 1); 

       msleep(100);

       gpiod_set_value(reset, 0);
       data->isp_mode = false;
       mutex_unlock(&data->lock);
    } else
        PINFO("wrong format \n");

    return len;
} 

static DEVICE_ATTR_WO(reset);

static struct attribute *device_attrs[] = {
	    &dev_attr_isp.attr,
        &dev_attr_reset.attr,
	    NULL
};
ATTRIBUTE_GROUPS(device);

/***************************/
/*****module init + exit****/
/***************************/

static int driver_probe (struct platform_device *pdev)
{
    int res;

    //printk(KERN_INFO "driver module init\n");
    PINFO ("driver module init\n");
    printk(KERN_DEBUG "fuckkkkkkkkkkkkkkkkkkkk\n");

    /*
    struct device_node * pDN;
    pDN = of_find_node_by_name(NULL, pdev->name);
    if (!pDN)
        PERR("device node pointer is NULL\n");

    int count = of_gpio_named_count(pDN, "reset-gpios");
    PINFO("gpio count is: %d \n", count);
    */
    
    PINFO ("node name %s\n",pdev->dev.of_node->name );

    // register a device with major and minor number without create device file
    res = alloc_chrdev_region(&device_num, FIRST_MINOR, 250, DRIVER_NAME); 
    if (res){
        PERR("Can't register driver, error code: %d \n", res); 
        goto error;
    }
    PINFO("Succes register driver with major is %d, minor is %d \n", MAJOR(device_num), MINOR(device_num));
    

    // create class 
    
    device_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(device_class))
    {
        PERR("Class create faili, error code: %d\n", device_class);

        goto error_class;
    }

    // create private data
    data = (private_data_t*)kcalloc(1, sizeof(private_data_t), GFP_KERNEL);
    data->isp_mode = false;
    mutex_init(&data->lock);
    
    // create device and add attribute simultaneously
    device = device_create_with_groups(device_class, NULL, device_num, data, device_groups, DRIVER_NAME);
    if (IS_ERR(device))
    {
        PERR("device create fall, error code: %d\n", device);

        goto error_device;
    }

    reset =  gpiod_get(&pdev->dev, "reset", GPIOD_OUT_LOW);
    if (IS_ERR(reset))
    {
        PINFO ("can't get reset gpio, error code: %d\n", reset);

        goto error_reset_gpio;
    }

    isp =  gpiod_get(&pdev->dev, "isp", GPIOD_OUT_HIGH);
    if (IS_ERR(isp))
    {
        PINFO ("can't get isp gpio, error code: %d\n", isp);

        goto error_isp_gpio;
    }

    //init gpio
    res = gpiod_direction_output(reset, 0);
    if (res)
    {
        PERR ("reset gpio can't set as output, error code: %d", res); 

        goto error_gpio;   
    }

    res = gpiod_direction_output(isp, 1);
    if (res)
    {
        PERR ("isp gpio can't set as output, error code: %d", res); 

        goto error_gpio;   
    }
    return 0;

    //error handle
error_gpio:
    gpiod_put(isp);
error_isp_gpio:
    gpiod_put(reset);
error_reset_gpio:
    device_destroy(device_class, device_num);
error_device:
    class_destroy(device_class);
error_class:
    unregister_chrdev_region(device_num, FIRST_MINOR); 
error:
    return -1;

}

static int driver_remove(struct platform_device *pdev)
{
    //printk(KERN_INFO "driver module exit\n");
    PINFO("driver module remove from kernel\n");
    
    gpiod_put(reset);
    gpiod_put(isp);

    device_destroy(device_class, device_num);
    class_destroy(device_class);
    unregister_chrdev_region(device_num, FIRST_MINOR); 
    kfree(data);

    return 0;
}

module_platform_driver(reset_driver);
