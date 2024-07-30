
# Cliente BLE SPP Rinho

Este proyecto es un terminal serial para conectarse a los dispositivos Rinho Telematics, específicamente al [Rinho Spider IoT](https://www.rinho.com.ar/productos/rinho-spider-iot) y [Rinho Smart IoT](https://www.rinho.com.ar/productos/rinho-smart-iot). Utiliza los UUID de Nordic para la comunicación BLE y hace uso de la biblioteca NimBLE para una gestión eficiente de Bluetooth Low Energy.

## Descripción

El Cliente BLE SPP Rinho permite la conexión a dispositivos Rinho Telematics a través de BLE (Bluetooth Low Energy). El propósito principal es proporcionar una interfaz de terminal serial que facilite la comunicación con estos dispositivos para enviar comandos y recibir datos.

## Dispositivos Soportados

- [Rinho Spider IoT](https://www.rinho.com.ar/productos/rinho-spider-iot)
- [Rinho Smart IoT](https://www.rinho.com.ar/productos/rinho-smart-iot)

## Página Oficial

Para más información sobre los productos y la empresa, visita la [página oficial de Rinho](https://www.rinho.com.ar).

## Bibliotecas Utilizadas

- **NimBLE**: Utilizamos la biblioteca NimBLE para una gestión eficiente de las conexiones Bluetooth Low Energy. NimBLE es conocida por su menor consumo de recursos y mayor rendimiento en comparación con otras bibliotecas BLE.

## Configuración de PlatformIO

Para configurar tu entorno de desarrollo con PlatformIO y NimBLE, sigue estos pasos:

1. **Abrir el archivo `platformio.ini` en tu proyecto.**
2. **Agregar la dependencia de la biblioteca NimBLE.**

Tu archivo `platformio.ini` debería verse así:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    h2zero/NimBLE-Arduino @ ^1.3.1
```

## Comandos Disponibles

- `$SCAN`: Escanea dispositivos BLE cercanos.
- `$CONNECT n`: Conecta al dispositivo en el índice `n` de la lista de dispositivos escaneados.
- `$CONNECT`: Reconecta al último dispositivo conectado.
- `$CLOSE`: Desconecta del servidor y detiene la reconexión automática.
- `$RESET`: Reinicia el dispositivo.
- `$FACTORY`: Realiza un reinicio de fábrica del dispositivo.

## Funcionamiento

1. **Inicio**: Al iniciar el dispositivo, se intentará conectar automáticamente al último dispositivo conectado guardado en las preferencias. Si no hay un dispositivo guardado, estará listo para escanear nuevos dispositivos.
2. **Escaneo**: Usa el comando `$SCAN` para escanear dispositivos BLE cercanos. Los dispositivos encontrados se listarán con un índice.
3. **Conexión**: Conéctate a un dispositivo especificando su índice con `$CONNECT n`. También puedes reconectar al último dispositivo conectado simplemente usando `$CONNECT`.
4. **Desconexión**: Usa `$CLOSE` para desconectar y detener la reconexión automática. Esto también eliminará la dirección del dispositivo guardado.
5. **Reinicio**: Reinicia el dispositivo con `$RESET` o realiza un reinicio de fábrica con `$FACTORY`.

## UUIDs Utilizados

El cliente BLE SPP Rinho utiliza los siguientes UUIDs de Nordic:

- **Servicio UART**: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
- **Característica RX**: `6e400003-b5a3-f393-e0a9-e50e24dcca9e`
- **Característica TX**: `6e400002-b5a3-f393-e0a9-e50e24dcca9e`

## Ejemplo de Uso

1. **Escanear dispositivos**:
   ```sh
   $SCAN
   ```
   Salida esperada:
   ```
   Escaneando dispositivos...
   Índice: 0, Nombre: Rinho Spider IoT, Dirección: XX:XX:XX:XX:XX:XX
   Índice: 1, Nombre: Rinho Smart IoT, Dirección: XX:XX:XX:XX:XX:XX
   ```

2. **Conectar a un dispositivo**:
   ```sh
   $CONNECT 0
   ```
   Salida esperada:
   ```
   Conectando al dispositivo en el índice 0...
   Conectado exitosamente
   ```

3. **Enviar comando al dispositivo conectado**:
   ```sh
   >QIO<
   ```
   Salida esperada:
   ```
   >RIO;IGN1;IN1111111;XP000;V122;VBU424;ID=865413050809192;*13<
   ```

4. **Desconectar del dispositivo**:
   ```sh
   $CLOSE
   ```
   Salida esperada:
   ```
   Desconectado del servidor
   ```

5. **Reiniciar el dispositivo**:
   ```sh
   $RESET
   ```

6. **Reinicio de fábrica**:
   ```sh
   $FACTORY
   ```

## Contribuciones

Las contribuciones son bienvenidas. Siéntete libre de abrir un issue o enviar un pull request.

## Licencia

Este proyecto está licenciado bajo la MIT License.
