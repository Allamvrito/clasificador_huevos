# Clasificador de Huevos

Este repositorio contiene el código para un sistema automatizado de clasificación de huevos según su peso. El proyecto se compone de dos partes principales:

1. **Código para Arduino**: Se encarga de la lectura del peso de los huevos, control de servomotores y clasificación de los huevos.
2. **Aplicación en Python**: Funciona como servidor para recibir los datos del Arduino, mostrarlos en tiempo real y almacenarlos en una base de datos.

## Estructura del Repositorio

### Carpeta `clasificador`
- **`app2.py`**: Script en Python que actúa como servidor para recibir los datos del Arduino, mostrar la información en una interfaz web y almacenar los registros en la base de datos.
- **`app.db`**: Base de datos SQLite donde se guardan los datos de clasificación de los huevos.

### Carpeta `prueba1.26` (Código para Arduino)
- **`prueba1.26.ino`**: Código principal para Arduino que gestiona los servos, sensor de peso y comunicaciones con el servidor.
- **`arduino_secrets.h`**: Archivo de configuración con credenciales o ajustes sensibles para la conexión WiFi y otros parámetros.

## 🛠️ Descripción del Código Arduino

### 🔹 `prueba1.26.ino`
Este código es la primera versión funcional del clasificador de huevos. Se encarga de:

1. **Leer el peso del huevo** usando el módulo **HX711**.
2. **Clasificar el huevo** en una de las siguientes categorías según su peso:
   - **Menos de 10g** → Huevos pequeños.
   - **10g a 20g** → Huevos medianos.
   - **20g a 30g** → Huevos grandes.
   - **Más de 30g** → Huevos jumbo.
3. **Mover los servomotores** para redirigir el huevo a la categoría correspondiente.
5. **Versión con Servidor Local**: Crea una interfaz web accesible en la red local

---

### 🔹 `prueba1.35.ino`
Esta versión mejora el código anterior con:
- **Pantalla OLED** para mostrar el peso y la categoría del huevo en tiempo real.
- **Mejor manejo de servomotores**, reduciendo el tiempo de espera y optimizando la clasificación.
- **WiFi optimizado** con la librería `WiFiS3` para mejorar la estabilidad de conexión.
- **Lógica de clasificación más eficiente**, haciendo que el sistema sea más rápido y preciso.
- **Versión con Servidor Remoto**: Envía datos a un servidor externo.


## Requisitos

### Para el código de Arduino:
- Placa Arduino compatible (por ejemplo, **Arduino Uno R4 WiFi** o **ESP32**).
- Sensor de peso con módulo **HX711**.
- Tres servomotores para la clasificación de los huevos.
- Conexión WiFi (si se requiere comunicación con el servidor).
- Librerías necesarias para el sensor de peso y la comunicación WiFi.

### Para la aplicación en Python:
- **Python 3.8+** (se recomienda **3.10+**).
- **sqlite3** (incluido por defecto en Python, no requiere instalación adicional).
- **pip** (para gestionar dependencias de Python).
- **Flask** (para crear el servidor web y manejar las rutas).

## 📜 Endpoints Disponibles

| Método | Ruta       | Descripción |
|--------|------------|-------------|
| GET    | /data      | Retorna los datos actuales del sensor en formato JSON. |
| POST   | /update    | Recibe datos en formato JSON y los almacena en la base de datos. |
| GET    | /          | Página principal que muestra los datos en tiempo real. |
| GET    | /list      | Lista todos los registros almacenados en la base de datos. |

## Instrucciones de Uso

### 1. Configurar y cargar el código en Arduino
- Abre el archivo `arduino_secrets.h` y ajusta la configuración necesaria para tu red WiFi y otros parámetros del proyecto.
- Carga el archivo `prueba1.26.ino` en tu placa Arduino.

### 2. Ejecutar la aplicación en Python
- Instala las dependencias necesarias usando `pip` (si no lo has hecho ya):
- Luego ejecuta el servidor Python:
  ```bash
  python app2.py

