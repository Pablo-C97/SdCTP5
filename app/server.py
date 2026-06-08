import math
import time
import os
import threading
import fcntl
from datetime import datetime
from flask import Flask, jsonify, request, send_from_directory

app = Flask(__name__)

# Configuración de muestreo
SAMPLING_HZ = 20  
SAMPLE_INTERVAL = 1.0 / SAMPLING_HZ

DEVICE_PATH = "/dev/cdd_sensor"

# Estado global y buffers de adquisición paralela
# Estado global y BUFFER ÚNICO de adquisición real
current_signal = 0  # 0 para Canal A, 1 para Canal B
buffer_datos = []      
buffer_lock = threading.Lock()
start_time = time.time()


def background_sampler():
    global buffer_datos, current_signal
    print(f"[Driver Real] Adquisición de ALTA VELOCIDAD iniciada a {SAMPLING_HZ} Hz...")
    
    while not os.path.exists(DEVICE_PATH):
        print("[ALERTA] Esperando a que el driver /dev/cdd_sensor esté disponible...")
        time.sleep(1)

    try:
        # Abrimos en modo binario R/W permanente sin buffer
        with open(DEVICE_PATH, "r+b", buffering=0) as fd:
            
            while True:
                start_loop = time.time()
                timestamp = datetime.now().strftime('%H:%M:%S.%f')[:-4]
                
                try:
                    # 1. CONMUTAR CANAL
                    fcntl.ioctl(fd, 0, current_signal)
                    
                    # 2. LEER DATOS CRUDOS (Sin seek, directo al driver)
                    raw_bytes = fd.read(16)
                    
                    if raw_bytes:
                        raw_line = raw_bytes.decode('utf-8').strip()
                        if raw_line:
                            real_value = int(raw_line)
                        else:
                            continue
                    else:
                        continue

                    nombre_canal = "Canal A" if current_signal == 0 else "Canal B"

                    # 3. GUARDAR EN EL BUFFER ÚNICO
                    with buffer_lock:
                        buffer_datos.append({
                            "time": timestamp, 
                            "value": real_value, 
                            "signal_type": nombre_canal
                        })
                        if len(buffer_datos) > 1500: 
                            buffer_datos.pop(0)

                except Exception as e:
                    print(f"[ERROR EN CICLO]: {e}")

                # 4. COMPENSACIÓN DE TIEMPO REAL
                elapsed = time.time() - start_loop
                sleep_time = SAMPLE_INTERVAL - elapsed
                if sleep_time > 0:
                    time.sleep(sleep_time)

    except Exception as e:
        print(f"[FATAL - DRIVER CERRADO]: {e}")
        
@app.route('/')
def index():
    return send_from_directory('.', 'index.html')

# INSTRUCCIONES DE INTEGRACIÓN EN RASPBERRY PI:
# 1. Eliminar/Comentar el hilo secundario 't' y la función 'background_sampler'.
# 2. Modificar 'get_data()' para abrir, escribir el ID de canal y leer de /dev/cdd_sensor.
# 3. Retornar la muestra real en formato JSON: {"status": "success", "samples": [sample]}.
# El método GET ahora devuelve la lista única con toda la cronología
@app.route('/api/data', methods=['GET'])
def get_data():
    with buffer_lock:
        # Hacemos una copia de la lista para enviarla de forma segura mediante JSON
        samples_to_send = list(buffer_datos)

    # Devolvemos la estructura exacta que tu frontend espera en la clave 'samples'
    return jsonify({
        "status": "success",
        "samples": samples_to_send
    })

@app.route('/api/switch', methods=['POST'])
def switch_signal():
    global current_signal
    req_data = request.get_json()
    target = req_data.get('signal')
    
    if target in [0, 1, '0', '1']:
        current_signal = int(target)
        canal_nombre = "A" if current_signal == 0 else "B"
        return jsonify({"status": "success", "message": f"Conmutado a canal {canal_nombre}"})
    return jsonify({"status": "error", "message": "Señal inválida"}), 400

if __name__ == '__main__':
    t = threading.Thread(target=background_sampler, daemon=True)
    t.start()
    app.run(host='0.0.0.0', port=5000, debug=False)