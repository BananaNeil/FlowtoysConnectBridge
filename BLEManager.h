#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class BLEManager
{
  public:

    class ServerCallback: public BLEServerCallbacks {
      public:
        ServerCallback(BLEManager * manager)   : manager(manager) {}
        BLEManager * manager;

        void onConnect(BLEServer* pServer) {
          manager->deviceConnected = true;
          DBG("BLE is connected.");
        };

        void onDisconnect(BLEServer* pServer) {
          manager->deviceConnected = false;
          DBG("BLE is disconnected.");
        }
    };

    class DataCallback: public BLECharacteristicCallbacks {
        void onRead(BLECharacteristic *pCharacteristic) {
          DBG("[BLE] onRead...");
        }
        void onWrite(BLECharacteristic *pCharacteristic) {
          DBG("[BLE] onWrite...");
          String message = String(pCharacteristic->getValue().c_str());
          DBG("[BLE] Received " + message);
          SerialManager::instance->parseMessage(message);
        }
    };

    // before setup()
    static void my_gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param) {
      DBG("GATTC event handler");
      // ESP_LOGW(LOG_TAG, "custom gattc event handler, event: %d", (uint8_t)event);
    }
    static void my_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gatts_cb_param_t* param) {
      DBG("GATTS event handler");
    }
    static void my_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
      DBG("GAP event handler");
    }


    BLEManager() {}

    BLEServer *pServer = NULL;
    BLECharacteristic * pTxCharacteristic;
    BLECharacteristic * pRxCharacteristic;
    bool deviceConnected = false;
    bool oldDeviceConnected = false;
    uint8_t txValue = 0;

    bool isActivated;
    
    void init()
    {
      // Create the BLE Device

      isActivated = Config::instance->getBLEMode();

      if(!isActivated)
      {
        DBG("BLE not activated in preferences, not initializing");
        return;
      }
      

      BLEDevice::setCustomGattcHandler(my_gattc_event_handler);
      BLEDevice::setCustomGattsHandler(my_gatts_event_handler);
      BLEDevice::setCustomGapHandler(my_gap_event_handler);

      String bleName = Config::instance->getDeviceName();
      BLEDevice::init(bleName.c_str());

      // Create the BLE Server
      pServer = BLEDevice::createServer();
      pServer->setCallbacks(new ServerCallback(this));



      // Create the BLE Service
      BLEService *pService = pServer->createService(SERVICE_UUID);

      // Create a BLE Characteristic
      pTxCharacteristic = pService->createCharacteristic(
                            CHARACTERISTIC_UUID_TX,
                            BLECharacteristic::PROPERTY_NOTIFY
                          );

      pTxCharacteristic->addDescriptor(new BLE2902());
      pTxCharacteristic->setCallbacks(new DataCallback());

      pRxCharacteristic = pService->createCharacteristic(
          CHARACTERISTIC_UUID_RX,
          BLECharacteristic::PROPERTY_WRITE_NR
                                              );

      pRxCharacteristic->setCallbacks(new DataCallback());

      // Start the service
      pService->start();

      // Start advertising
      pServer->getAdvertising()->start();
      DBG("BLE is init with name "+bleName);
    }

		void sendString(char* command) {
      if (deviceConnected)  {
				// DBG("Send command via BLE: " + command);
        // Serial.printf("*** Sent Value: %d ***\n", 
        //     txValue);
         pTxCharacteristic->setValue(command); 
         pTxCharacteristic->notify(); 
         // txValue++; 
      }
		}

		void sendBytes(void *bytePointer, size_t size, char messageType) {
      if (deviceConnected)  {
        uint8_t *bytes = new uint8_t[size+1];
        bytes = (unsigned uint8_t*)bytePointer;
        bytes[size] = messageType;
        pTxCharacteristic->setValue(bytes, size+1); 
        pTxCharacteristic->notify(); 
      }
		}

    void update()
    {
      if(!isActivated) return;

      
      if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
      }
      // connecting
      if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
      }
    }
};
