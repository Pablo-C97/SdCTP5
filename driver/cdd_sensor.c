#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>

static int __init sensor_init(void) {
    printk(KERN_INFO "SDCTP5: Driver inicializado\n");
    return 0;
}

static void __exit sensor_exit(void) {
    printk(KERN_INFO "SDCTP5: Driver removido\n");
}

module_init(sensor_init);
module_exit(sensor_exit);

MODULE_LICENSE("GPL");