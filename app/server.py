import math
import time
import threading
from datetime import datetime
from flask import Flask, jsonify, request, send_from_directory

app = Flask(__name__)

# Configuración de muestreo
SAMPLING_HZ = 20  
SAMPLE_INTERVAL = 1.0 / SAMPLING_HZ

# Estado global y buffers de adquisición paralela
current_signal = 0
buffer_A = []      
buffer_B = []      
buffer_lock = threading.Lock()
start_time = time.time()

def background_sampler():
    global buffer_A, buffer_B, start_time
    print(f"[Kernel Sim] Adquisición PARALELA iniciada a {SAMPLING_HZ} Hz...")
    
    while True:
        t_current = time.time()
        elapsed = t_current - start_time
        timestamp = datetime.fromtimestamp(t_current).strftime('%H:%M:%S.%f')[:-4]
        
        # Simulación de señales (Canal A: Analógica, Canal B: Digital)
        value_A = round(4.0 * math.sin(elapsed * 1.5), 2)
        value_B = 3.3 if (int(elapsed * 3) % 2 == 0) else 0.0
        
        with buffer_lock:
            buffer_A.append({"time": timestamp, "value": value_A, "signal_type": "Señal A"})
            buffer_B.append({"time": timestamp, "value": value_B, "signal_type": "Señal B"})
            
            if len(buffer_A) > 1500: buffer_A.pop(0)
            if len(buffer_B) > 1500: buffer_B.pop(0)
                
        time.sleep(SAMPLE_INTERVAL)

@app.route('/')
def index():
    return send_from_directory('.', 'index.html')

# INSTRUCCIONES DE INTEGRACIÓN EN RASPBERRY PI:
# 1. Eliminar/Comentar el hilo secundario 't' y la función 'background_sampler'.
# 2. Modificar 'get_data()' para abrir, escribir el ID de canal y leer de /dev/cdd_sensor.
# 3. Retornar la muestra real en formato JSON: {"status": "success", "samples": [sample]}.
@app.route('/api/data', methods=['GET'])
def get_data():
    signal_requested = request.args.get('signal', default=0, type=int)
    
    with buffer_lock:
        if signal_requested == 0:
            samples_packet = list(buffer_A)
        else:
            samples_packet = list(buffer_B)
        
    return jsonify({
        "status": "success",
        "samples": samples_packet
    })

@app.route('/api/switch', methods=['POST'])
def switch_signal():
    global current_signal
    req_data = request.get_json()
    target = req_data.get('signal')
    
    if target in [0, 1, '0', '1']:
        current_signal = int(target)
        return jsonify({"status": "success", "message": f"Conmutado a canal {current_signal}"})
    return jsonify({"status": "error", "message": "Señal inválida"}), 400

if __name__ == '__main__':
    t = threading.Thread(target=background_sampler, daemon=True)
    t.start()
    app.run(host='0.0.0.0', port=5000, debug=False)