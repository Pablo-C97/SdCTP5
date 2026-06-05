#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "cdd_sensor"
#define CLASS_NAME "sensor_class"

static dev_t dev_num;
static struct cdev sensor_cdev;
static struct class *sensor_class = NULL;
static struct device *sensor_device = NULL;

// Prototipos de las operaciones del driver
static int sensor_open(struct inode *inode, struct file *file);
static int sensor_release(struct inode *inode, struct file *file);
static ssize_t sensor_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);
static long sensor_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

// Estructura file_operations
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = sensor_open,
    .release = sensor_release,
    .read = sensor_read,
    .unlocked_ioctl = sensor_ioctl,
};

// Implementación de los callbacks

static int sensor_open(struct inode *inode, struct file *file) {
    pr_info("SDCTP5: Dispositivo abierto por el espacio de usuario\n");
    return 0;
}

static int sensor_release(struct inode *inode, struct file *file) {
    pr_info("SDCTP5: Dispositivo cerrado\n");
    return 0;
}

static ssize_t sensor_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset) {
    // TODO: Implementar lectura de GPIO y uso de copy_to_user() para enviar el dato al server.py
    pr_info("SDCTP5: Petición de lectura recibida\n");
    
    // Retornamos 0 temporalmente para indicar EOF y que no falle la compilación
    return 0; 
}

static long sensor_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // TODO: Implementar lógica (switch/case) para alternar entre Señal A y Señal B
    pr_info("SDCTP5: IOCTL recibido con cmd = %u\n", cmd);
    return 0;
}

// Inicialización y Salida

static int __init sensor_init(void) {
    int ret;

    //  Asignación dinámica de Major y Minor
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("SDCTP5: Error al asignar Major/Minor\n");
        return ret;
    }
    pr_info("SDCTP5: Registrado exitosamente con Major: %d, Minor: %d\n", MAJOR(dev_num), MINOR(dev_num));

    // Inicialización y adición del cdev al kernel
    cdev_init(&sensor_cdev, &fops);
    ret = cdev_add(&sensor_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("SDCTP5: Error al agregar cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    // Creación automática del nodo en /dev/
    sensor_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(sensor_class)) {
        pr_err("SDCTP5: Error al crear la clase\n");
        cdev_del(&sensor_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(sensor_class);
    }

    sensor_device = device_create(sensor_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(sensor_device)) {
        pr_err("SDCTP5: Error al crear el dispositivo\n");
        class_destroy(sensor_class);
        cdev_del(&sensor_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(sensor_device);
    }

    pr_info("SDCTP5: Driver inicializado y nodo creado automáticamente en /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit sensor_exit(void) {
    // La limpieza debe hacerse en orden estrictamente inverso a la inicialización
    device_destroy(sensor_class, dev_num);
    class_destroy(sensor_class);
    cdev_del(&sensor_cdev);
    unregister_chrdev_region(dev_num, 1);
    
    pr_info("SDCTP5: Driver removido exitosamente\n");
}

module_init(sensor_init);
module_exit(sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KernelPanics");
MODULE_DESCRIPTION("Character Device Driver para adquisición de señales");
