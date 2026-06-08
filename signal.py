import sys
import time
import threading
import RPi.GPIO as GPIO

# Configuración de Pines GPIO (Asegúrate de que coincidan con tu driver)
PIN_SENAL_A = 26  # para la primera señal cuadrada
PIN_SENAL_B = 19  # para la segunda señal cuadrada

def generar_onda_cuadrada(pin, frecuencia, nombre_canal):
    """Genera una señal cuadrada simétrica en un pin específico."""
    if frecuencia <= 0:
        print(f"[{nombre_canal}] Frecuencia inválida ({frecuencia} Hz). Canal apagado.")
        return

    # El periodo total es 1/f. Cada semiperiodo (ALTO o BAJO) dura la mitad.
    tiempo_estado = 1.0 / (2.0 * frecuencia)
    print(f"[{nombre_canal}] Iniciado en GPIO {pin} a {frecuencia} Hz (Ciclo: {tiempo_estado:.4f}s)")
    
    estado = GPIO.LOW
    while True:
        estado = GPIO.HIGH if estado == GPIO.LOW else GPIO.LOW
        GPIO.output(pin, estado)
        time.sleep(tiempo_estado)

if __name__ == '__main__':
    # Valores por defecto si el usuario no introduce parámetros
    frec_a = 1.0  # 2 Hz por defecto
    frec_b = 5.0  # 5 Hz por defecto

    # Leer parámetros desde la terminal (consola)
    if len(sys.argv) >= 3:
        try:
            frec_a = float(sys.argv[1])
            frec_b = float(sys.argv[2])
        except ValueError:
            print("Error: Las frecuencias deben ser números. Usando valores por defecto.")
    elif len(sys.argv) == 2:
        try:
            frec_a = float(sys.argv[1])
            print("Falta la segunda frecuencia. Usando valor por defecto para el Canal B.")
        except ValueError:
            print("Error: La frecuencia debe ser un número. Usando valores por defecto.")

    print("--- GENERADOR DE SEÑALES DIGITALES ---")
    print(f"Configurando Canal A a {frec_a} Hz y Canal B a {frec_b} Hz...")

    # Configuración del hardware de la Raspberry Pi
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(PIN_SENAL_A, GPIO.OUT, initial=GPIO.LOW)
    GPIO.setup(PIN_SENAL_B, GPIO.OUT, initial=GPIO.LOW)

    try:
        # Creamos hilos independientes para que cada frecuencia corra a su propio ritmo
        t_a = threading.Thread(target=generar_onda_cuadrada, args=(PIN_SENAL_A, frec_a, "Canal A"), daemon=True)
        t_b = threading.Thread(target=generar_onda_cuadrada, args=(PIN_SENAL_B, frec_b, "Canal B"), daemon=True)
        
        t_a.start()
        t_b.start()
        
        # Mantener el script principal corriendo
        while True:
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\n[Generador] Deteniendo generación de señales...")
    finally:
        GPIO.cleanup()
        print("[Generador] Pines liberados correctamente.")