#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>

#define CMD_SCAN "$SCAN"
#define CMD_CONNECT "$CONNECT"
#define CMD_DISCONNECT "$CLOSE"
#define CMD_RESET "$RESET"
#define CMD_FACTORY "$FACTORY"
#define UART_BAUD_RATE 115200
#define TIMEOUT_MS 1000 // Timeout for sending data in milliseconds
#define RECONNECT_RETRIES 3
#define RECONNECT_INTERVAL_MS 5000 // Interval between reconnection attempts

// Define los UUIDs de los servicios y características
static BLEUUID sv_uuid("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static BLEUUID rx_uuid("6e400003-b5a3-f393-e0a9-e50e24dcca9e");
static BLEUUID tx_uuid("6e400002-b5a3-f393-e0a9-e50e24dcca9e");

Preferences preferences;
BLEAdvertisedDevice *myDevice = nullptr;
NimBLEClient *pClient = nullptr;
NimBLERemoteCharacteristic *pRxCharacteristic = nullptr;
NimBLERemoteCharacteristic *pTxCharacteristic = nullptr;
std::vector<NimBLEAdvertisedDevice *> foundDevices;
bool connected = false;
bool shouldReconnect = false;

String serialBuffer = "";
String bleBuffer = ""; // Buffer para ensamblar los datos recibidos
unsigned long lastSerialTime = 0;
unsigned long lastReconnectAttempt = 0;

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) override {
        foundDevices.push_back(new NimBLEAdvertisedDevice(*advertisedDevice));
        Serial.print("Idx: ");
        Serial.print(foundDevices.size() - 1);
        Serial.print(", Dirección: ");
        Serial.print(advertisedDevice->getAddress().toString().c_str());
        if (advertisedDevice->haveName()) {
            Serial.print(", Nombre: ");
            Serial.print(advertisedDevice->getName().c_str());
        }
        Serial.println();
    }
};

class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient *pClient) override {
        Serial.println("Conectado al servidor");
    }

    void onDisconnect(NimBLEClient *pClient) override {
        Serial.println("Desconectado del servidor");
        connected = false;
        if (shouldReconnect) {
            Serial.println("Intentando reconectar...");
        }
    }
};

void startScan();
void connectToServer(NimBLEAddress address);
void disconnectFromServer();
bool connectWithRetries(NimBLEAddress address, int retries);
void listServicesAndCharacteristics(NimBLEClient *pClient);
void resetDevice();
void factoryResetDevice();
void handleCommand(String cmd);
void handleSerialData();
void handleBLEData(NimBLERemoteCharacteristic *pCharacteristic, uint8_t *pData, size_t length, bool isNotify);
void attemptReconnect();
void printIntroduction();

void setup() {
    Serial.begin(UART_BAUD_RATE);
    preferences.begin("ble_data", false);
    NimBLEDevice::init("");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Aumentar la potencia de la señal si es necesario

    printIntroduction();

    String savedAddress = preferences.getString("lastAddress", "");
    if (savedAddress.length() > 0) {
        NimBLEAddress addr(savedAddress.c_str());
        shouldReconnect = true;
        connectToServer(addr);
    }
}

void loop() {
    handleSerialData();
    if (!connected && shouldReconnect && millis() - lastReconnectAttempt > RECONNECT_INTERVAL_MS) {
        String savedAddress = preferences.getString("lastAddress", "");
        if (savedAddress.length() > 0) {
            NimBLEAddress addr(savedAddress.c_str());
            connectToServer(addr);
        }
        lastReconnectAttempt = millis();
    }
}

void printIntroduction() {
    Serial.println("============================================");
    Serial.println("   Cliente Rinho BLE SPP v0.1");
    Serial.println("============================================");
    Serial.println("Comandos disponibles:");
    Serial.println("$SCAN         - Escanear dispositivos BLE");
    Serial.println("$CONNECT n    - Conectar al dispositivo en el índice n");
    Serial.println("$CONNECT      - Reconectar al último dispositivo conectado");
    Serial.println("$CLOSE        - Desconectar del servidor y detener reconexión");
    Serial.println("$RESET        - Reiniciar el dispositivo");
    Serial.println("$FACTORY      - Realizar reinicio de fábrica");
    Serial.println("============================================");
}

void handleCommand(String cmd) {
    if (cmd.equalsIgnoreCase(CMD_SCAN)) {
        startScan();
    } else if (cmd.startsWith(CMD_CONNECT)) {
        int index = cmd.substring(strlen(CMD_CONNECT) + 1).toInt();
        if (index >= 0 && index < foundDevices.size()) {
            Serial.print("Conectando al dispositivo en el índice ");
            Serial.println(index);
            shouldReconnect = true;
            if (connectWithRetries(foundDevices[index]->getAddress(), RECONNECT_RETRIES)) {
                Serial.println("Conexión exitosa");
            } else {
                Serial.println("Falló la conexión después de varios intentos");
            }
        } else if (cmd.equalsIgnoreCase(CMD_CONNECT)) {
            String savedAddress = preferences.getString("lastAddress", "");
            if (savedAddress.length() > 0) {
                NimBLEAddress addr(savedAddress.c_str());
                Serial.println("Reconectando a la última dirección guardada...");
                shouldReconnect = true;
                connectToServer(addr);
            } else {
                Serial.println("No se encontró ninguna dirección guardada para reconectar");
            }
        } else {
            Serial.println("Índice de dispositivo no válido");
        }
    } else if (cmd.equalsIgnoreCase(CMD_DISCONNECT)) {
        shouldReconnect = false;
        preferences.remove("lastAddress"); // Eliminar dirección guardada
        disconnectFromServer();
    } else if (cmd.equalsIgnoreCase(CMD_RESET)) {
        resetDevice();
    } else if (cmd.equalsIgnoreCase(CMD_FACTORY)) {
        factoryResetDevice();
    } else if (connected && pTxCharacteristic != nullptr) {
        // Enviar el comando al dispositivo BLE si está conectado
        pTxCharacteristic->writeValue((const uint8_t *)cmd.c_str(), cmd.length(), false);
    } else {
        Serial.println("Comando desconocido o no conectado");
    }
}

void handleSerialData() {
    while (Serial.available()) {
        char c = Serial.read();
        serialBuffer += c;
        lastSerialTime = millis();

        // Enviar comando si está conectado y se recibe un carácter de nueva línea o se produce un tiempo de espera
        if (c == '\n') {
            serialBuffer.trim(); // Eliminar cualquier carácter de nueva línea o espacio adicional
            handleCommand(serialBuffer);
            serialBuffer = "";
        } else if (connected && pTxCharacteristic != nullptr && millis() - lastSerialTime > TIMEOUT_MS) {
            pTxCharacteristic->writeValue((const uint8_t *)serialBuffer.c_str(), serialBuffer.length(), false);
            serialBuffer = "";
        }
    }
}

void handleBLEData(NimBLERemoteCharacteristic *pCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
    // Ensamblar los datos recibidos en bleBuffer
    for (size_t i = 0; i < length; i++) {
        bleBuffer += (char)pData[i];
    }

    // Imprimir el buffer ensamblado cuando se recibe un mensaje completo
    Serial.println(bleBuffer);
    // Procesar el buffer según sea necesario y luego limpiarlo
    bleBuffer = "";
}

void startScan() {
    foundDevices.clear();
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setActiveScan(true);
    pScan->start(10, false);
    Serial.println("Escaneando dispositivos...");
}

bool connectWithRetries(NimBLEAddress address, int retries) {
    while (retries-- > 0) {
        Serial.print("Conectando a: ");
        Serial.println(address.toString().c_str());

        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks(), false);
        if (pClient->connect(address)) {
            Serial.println("Conectado, descubriendo servicios...");
            int mtu = pClient->getMTU(); // Obtener el MTU negociado
            Serial.print("MTU negociado: ");
            Serial.println(mtu);

            listServicesAndCharacteristics(pClient);
            NimBLERemoteService *pService = pClient->getService(sv_uuid);
            if (pService != nullptr) {
                pRxCharacteristic = pService->getCharacteristic(rx_uuid);
                pTxCharacteristic = pService->getCharacteristic(tx_uuid);
                if (pRxCharacteristic != nullptr && pTxCharacteristic != nullptr) {
                    pRxCharacteristic->subscribe(true, handleBLEData);
                    // Guardar dirección en preferencias
                    preferences.putString("lastAddress", address.toString().c_str());
                    Serial.println("Conectado al servidor y características encontradas");
                    connected = true;
                    return true;
                } else {
                    Serial.println("No se encontraron nuestras características");
                }
            } else {
                Serial.println("No se encontró nuestro UUID de servicio");
            }
        } else {
            Serial.println("Falló la conexión");
        }

        if (pClient->isConnected()) {
            pClient->disconnect();
        }

        delay(1000); // Retardo antes de volver a intentar
    }

    return false;
}

void listServicesAndCharacteristics(NimBLEClient *pClient) {
    Serial.println("Listando servicios y características disponibles...");
    std::vector<NimBLERemoteService *> *services = pClient->getServices();
    for (auto const &entry : *services) {
        Serial.print("UUID del servicio: ");
        Serial.println(entry->getUUID().toString().c_str());

        std::vector<NimBLERemoteCharacteristic *> *characteristics = entry->getCharacteristics();
        for (auto const &charEntry : *characteristics) {
            Serial.print("  UUID de la característica: ");
            Serial.println(charEntry->getUUID().toString().c_str());
        }
    }
}

void connectToServer(NimBLEAddress address) {
    if (connectWithRetries(address, RECONNECT_RETRIES)) {
        Serial.println("Conectado al servidor");
    } else {
        Serial.println("Falló la conexión al servidor");
    }
}

void disconnectFromServer() {
    if (pClient && pClient->isConnected()) {
        pClient->disconnect();
        connected = false;
    } else {
        Serial.println("No está conectado a ningún servidor");
    }
}

void resetDevice() {
    Serial.println("Reiniciando dispositivo...");
    delay(1000); // Retardo opcional para retroalimentación visual
    ESP.restart();
}

void factoryResetDevice() {
    Serial.println("Realizando reinicio de fábrica...");
    preferences.clear();
    disconnectFromServer();
    delay(1000); // Retardo opcional para retroalimentación visual
    ESP.restart();
}
