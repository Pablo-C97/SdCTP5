#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>  //  Añadir esta cabecera para interactuar con los pines


#define PIN_CANAL_A 532
#define PIN_CANAL_B 528

// Por defecto, empezamos escuchando el Canal A
static int gpio_actual = PIN_CANAL_A;

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
    pr_info("Dispositivo abierto por el espacio de usuario\n");
    return 0;
}

static int sensor_release(struct inode *inode, struct file *file) {
    pr_info("Dispositivo cerrado\n");
    return 0;
}
static ssize_t sensor_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset) {
    int pin_value;
    char buffer[16];
    int len;

 

    pr_info("Petición de lectura recibida desde el driver\n");

    // Leemos el valor real del pin seleccionado
    pin_value = gpio_get_value(gpio_actual);
    pr_info("CDD_SENSOR: Petición recibida. El valor del pin es: %d\n", pin_value);

    // Convertimos el entero a cadena de texto
    len = scnprintf(buffer, sizeof(buffer), "%d\n", pin_value);

    if (size < len) {
        return -EINVAL; 
    }

    if (copy_to_user(user_buffer, buffer, len) != 0) {
        pr_err("Error al enviar datos al espacio de usuario\n");
        return -EFAULT;
    }

    //  forzamos que el offset del kernel vuelva a ser 0 para la próxima lectura de Python.
    *offset = 0; 

    return len;
}

static long sensor_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // Usamos el argumento que envíe Python (0 para Canal A, 1 para Canal B)
    if (arg == 0) {
        gpio_actual = PIN_CANAL_A;
        pr_info("Driver cambiado al Canal A (GPIO %d)\n", PIN_CANAL_A);
    } else if (arg == 1) {
        gpio_actual = PIN_CANAL_B;
        pr_info("Driver cambiado al Canal B (GPIO %d)\n", PIN_CANAL_B);
    } else {
        return -EINVAL; // Canal no válido
    }
    pr_info("IOCTL recibido con cmd = %u\n", cmd);
    return 0;

}

// Inicialización y Salida

static int __init sensor_init(void) {
    int ret;

    //  Asignación dinámica de Major y Minor
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Error al asignar Major/Minor\n");
        return ret;
    }
    pr_info("Registrado exitosamente con Major: %d, Minor: %d\n", MAJOR(dev_num), MINOR(dev_num));

    // Inicialización y adición del cdev al kernel
    cdev_init(&sensor_cdev, &fops);
    ret = cdev_add(&sensor_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("Error al agregar cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    // Creación automática del nodo en /dev/
    sensor_class = class_create(CLASS_NAME);
    if (IS_ERR(sensor_class)) {
        pr_err("Error al crear la clase\n");
        cdev_del(&sensor_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(sensor_class);
    }

    sensor_device = device_create(sensor_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(sensor_device)) {
        pr_err("Error al crear el dispositivo\n");
        class_destroy(sensor_class);
        cdev_del(&sensor_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(sensor_device);
    }
    // Reservar y configurar el GPIO

// Reservar y configurar el Pin A con control de errores
    ret = gpio_request(PIN_CANAL_A, "sensor_input_A");
    if (ret < 0) {
        pr_err("Error al reservar PIN_CANAL_A\n");
        device_destroy(sensor_class, dev_num);
        class_destroy(sensor_class);
        cdev_del(&sensor_cdev);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    gpio_direction_input(PIN_CANAL_A);

    // Reservar y configurar el Pin B con control de errores
    ret = gpio_request(PIN_CANAL_B, "sensor_input_B");
    if (ret < 0) {
        pr_err("Error al reservar PIN_CANAL_B\n");
        gpio_free(PIN_CANAL_A); // Liberamos el A si el B falló
        device_destroy(sensor_class, dev_num);
        class_destroy(sensor_class);
        cdev_del(&sensor_cdev);
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    gpio_direction_input(PIN_CANAL_B);

    pr_info("Driver inicializado y nodo creado automáticamente en /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit sensor_exit(void) {
    // 1. Liberamos los pines GPIO REALES que reservamos
    gpio_free(PIN_CANAL_A);
    gpio_free(PIN_CANAL_B);
    // La limpieza debe hacerse en orden estrictamente inverso a la inicialización
    device_destroy(sensor_class, dev_num);
    class_destroy(sensor_class);
    cdev_del(&sensor_cdev);
    unregister_chrdev_region(dev_num, 1);
    //Liberar el pin digital


    pr_info("Driver removido exitosamente\n");
}

module_init(sensor_init);
module_exit(sensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KernelPanics");
MODULE_DESCRIPTION("Character Device Driver para adquisición de señales");
