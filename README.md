# SdCTP5
Trabajo Práctico N°5 - Sistemas de Computación Universidad Nacional de Córdoba (UNC) - FCEFyN

El presente trabajo aborda el diseño, la implementación y el despliegue de un sistema integral de adquisición de señales en tiempo real utilizando una plataforma embebida Raspberry Pi. El núcleo del proyecto radica en el desarrollo de un Driver de Dispositivo de Caracteres (CDD) integrado en el espacio de kernel de Linux, el cual actúa como puente directo con el hardware para gestionar la lectura física y la conmutación de señales externas a través de los pines GPIO.

* **Desarrollo y Compilación Cruzada:** Todo el código del controlador se escribió originalmente en una máquina anfitriona (*Host*). Para su construcción, se montó un entorno de compilación cruzada que implicó la descarga y sincronización rigurosa de las herramientas de compilación y las cabeceras del código fuente del kernel correspondientes a la versión exacta de la placa destino.
* **Diseño del Sistema de Visualización Remota:** Ante la ausencia de un entorno gráfico nativo en la placa, se diseñó una arquitectura de software distribuida en red. Esta consta de un servicio de *backend* (en Python) encargado de consumir de forma directa los datos del driver y un *frontend* interactivo (en HTML/JavaScript) que procesa los gráficos de forma remota en el navegador de la PC, calculando métricas de frecuencia y voltaje en tiempo real.
* **Despliegue y Pruebas Integrales:** Los binarios y scripts se transfirieron a la placa mediante protocolos de red seguros. El despliegue final contempló la carga del módulo en el kernel, la generación de señales de prueba conectadas físicamente a las entradas del sistema y la validación de la interfaz web para el control bidireccional de la adquisición.

