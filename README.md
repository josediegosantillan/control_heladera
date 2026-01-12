# Control Heladera

Proyecto ESP32 para monitoreo y control de heladeras mediante WiFi y MQTT.

## Componentes del Proyecto

- **ds18b20**: Sensor de temperatura DS18B20
- **mqtt_connector**: Conector MQTT para comunicación remota
- **wifi_portal**: Portal WiFi para configuración de conexión
- **zmct103c**: Sensor de corriente ZMCT103C para monitoreo energético

## Requisitos

- **ESP-IDF** v5.0 o superior
- **Python 3.7+**
- **CMake 3.16+**
- Herramientas de compilación (gcc, etc.)

## Instalación

### 1. Clonar el repositorio

```bash
git clone https://github.com/josediegosantillan/control_heladera.git
cd control_heladera
```

### 2. Configurar ESP-IDF

```bash
idf.py set-target esp32
idf.py menuconfig
```

### 3. Compilar el proyecto

```bash
idf.py build
```

### 4. Cargar en el dispositivo

```bash
idf.py flash
```

### 5. Monitor de salida

```bash
idf.py monitor
```

## Uso

Una vez compilado y cargado en el ESP32:

1. El dispositivo se conectará a WiFi según la configuración del portal.
2. Se conectará al broker MQTT configurado.
3. Publicará datos de temperatura (DS18B20) y consumo de corriente (ZMCT103C).

## Configuración

Edita los parámetros en `idf.py menuconfig` o directamente en `sdkconfig` para:

- SSID y contraseña WiFi
- URL y credenciales del broker MQTT
- Tópicos MQTT para publicar/suscribirse
- Configuración de sensores

## Estructura del Proyecto

```
.
├── main/                 # Código principal
│   ├── main.c
│   └── CMakeLists.txt
├── components/           # Componentes reutilizables
│   ├── ds18b20/          # Driver del sensor de temperatura
│   ├── mqtt_connector/   # Conectividad MQTT
│   ├── wifi_portal/      # Portal de configuración WiFi
│   └── zmct103c/         # Driver del sensor de corriente
├── CMakeLists.txt        # Configuración de compilación
└── README.md             # Este archivo
```

## Licencia

Este proyecto es de código abierto. Consulta la licencia para más detalles.

## Autor

Diego Santillán

---

Para más información sobre ESP-IDF, visita [https://docs.espressif.com/](https://docs.espressif.com/)
