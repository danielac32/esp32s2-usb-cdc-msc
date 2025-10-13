 
#include "USB.h"
#include "USBMSC.h"
#include "USBCDC.h"
USBCDC USBSerial;
USBMSC MSC;
const int ledPin =  15;
bool usbReady = false;
bool mscReady = false;

#define MSC_SECTOR_SIZE 512
#define MSC_SECTOR_COUNT 3072  // 1.5m

uint8_t* mscStorage = NULL;

 
static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == ARDUINO_USB_EVENTS) {
    arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT: 
        Serial.println("USB PLUGGED");
        usbReady = true;
        mscReady =true;
        break;
      case ARDUINO_USB_STOPPED_EVENT: 
        Serial.println("USB UNPLUGGED");
        usbReady = false;
        mscReady = false;
        break;
      case ARDUINO_USB_SUSPEND_EVENT: 
        Serial.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en); 
        break;
      case ARDUINO_USB_RESUME_EVENT:  
        Serial.println("USB RESUMED"); 
        break;
      default: 
        break;
    }
  }
}
 

void createBootSector() {
  // Crear un Master Boot Record (MBR) básico
  // Esto hace que el dispositivo sea reconocido como disco válido
  if (mscStorage) {
    // Signature al final del MBR
    mscStorage[510] = 0x55;
    mscStorage[511] = 0xAA;
    
    // Partition table entry (una partición que ocupa todo el disco)
    uint32_t totalSectors = MSC_SECTOR_COUNT;
    
    // Primera entrada de tabla de particiones en offset 0x1BE
    mscStorage[0x1BE] = 0x80; // Bootable
    mscStorage[0x1BF] = 0x00; // Starting head
    mscStorage[0x1C0] = 0x01; // Starting sector (bits 0-5)
    mscStorage[0x1C1] = 0x00; // Starting cylinder
    
    mscStorage[0x1C2] = 0x0C; // FAT32 LBA partition type
    mscStorage[0x1C3] = 0xFE; // Ending head
    mscStorage[0x1C4] = 0xFF; // Ending sector (bits 0-5) and cylinder (bits 6-15)
    mscStorage[0x1C5] = 0xFF; // Ending cylinder
    
    // LBA inicio (little endian)
    mscStorage[0x1C6] = 0x01;
    mscStorage[0x1C7] = 0x00;
    mscStorage[0x1C8] = 0x00;
    mscStorage[0x1C9] = 0x00;
    
    // Número de sectores (little endian)
    mscStorage[0x1CA] = (totalSectors - 1) & 0xFF;
    mscStorage[0x1CB] = ((totalSectors - 1) >> 8) & 0xFF;
    mscStorage[0x1CC] = ((totalSectors - 1) >> 16) & 0xFF;
    mscStorage[0x1CD] = ((totalSectors - 1) >> 24) & 0xFF;
    
    USBSerial.println("MBR creado en sector de arranque");
  }
}


static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  if (!mscStorage || lba >= MSC_SECTOR_COUNT) {
    return 0;
  }
  
  uint32_t storage_offset = lba * MSC_SECTOR_SIZE + offset;
  uint32_t bytes_to_copy = min(bufsize, (MSC_SECTOR_COUNT * MSC_SECTOR_SIZE) - storage_offset);
  
  if (bytes_to_copy > 0) {
    memcpy(buffer, mscStorage + storage_offset, bytes_to_copy);
  }
  
  return bytes_to_copy;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  if (!mscStorage || lba >= MSC_SECTOR_COUNT) {
    return 0;
  }
  
  uint32_t storage_offset = lba * MSC_SECTOR_SIZE + offset;
  uint32_t bytes_to_copy = min(bufsize, (MSC_SECTOR_COUNT * MSC_SECTOR_SIZE) - storage_offset);
  
  if (bytes_to_copy > 0) {
    memcpy(mscStorage + storage_offset, buffer, bytes_to_copy);
  }
  
  return bytes_to_copy;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  USBSerial.printf("MSC Start/Stop: power_condition=%d, start=%d, load_eject=%d\n", 
                power_condition, start, load_eject);
  mscReady = start;
  return true;
}



void setup() {
  pinMode(ledPin,OUTPUT);
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  if (psramFound()) {
    Serial.printf("PSRAM disponible: %d MB\n", ESP.getPsramSize() / 1048576);
    
    // Asignar memoria PSRAM para MSC
    mscStorage = (uint8_t*)ps_malloc(MSC_SECTOR_SIZE * MSC_SECTOR_COUNT);
    
    if (mscStorage) {
      Serial.printf("MSC Storage asignado: %d sectores (%d bytes)\n", 
                   MSC_SECTOR_COUNT, MSC_SECTOR_SIZE * MSC_SECTOR_COUNT);
      
      // Inicializar con datos de ejemplo (opcional)
      memset(mscStorage, 0, MSC_SECTOR_SIZE * MSC_SECTOR_COUNT);
      
      // Crear un sistema de archivos FAT simple en el primer sector
      // Esto ayuda a que Windows/Mac reconozca el dispositivo
      createBootSector();
      
    } else {
      Serial.println("Error: No se pudo asignar PSRAM para MSC");
      return;
    }
  } else {
    Serial.println("Error: PSRAM no disponible");
    return;
  }

  
  USBSerial.begin();
  MSC.vendorID("ESP32");
  MSC.productID("USB_MSC_RAM");
  MSC.productRevision("1.0");
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.onStartStop(onStartStop);
  MSC.mediaPresent(true);
  MSC.begin(MSC_SECTOR_COUNT, MSC_SECTOR_SIZE);

  USB.onEvent(usbEventCallback);
  //MSC.onEvent(mscEventCallback);
  USB.begin();
  

  while(!usbReady) {
    delay(100);
    USBSerial.println("Esperando USB...");
  }

 
  
  Serial.println("USB conectado - Creando tareas...");
  
  xTaskCreate(blinkTask, "BlinkTask", 2048, NULL, 1, NULL);
  xTaskCreate(serialTask, "SerialTask", 2048, NULL, 1, NULL);
  xTaskCreate(mscStatusTask, "MSCStatus", 2048, NULL, 1, NULL);
  
  Serial.println("Tareas creadas - Sistema listo");
}

void loop() {
  delay(1000);

}




void blinkTask(void *pvParameters) {
  while (1) {
    digitalWrite(ledPin, !digitalRead(ledPin));
    
    if (usbReady) {
      vTaskDelay(100 / portTICK_PERIOD_MS); // Rápido cuando MSC activo
    } else {
      vTaskDelay(500 / portTICK_PERIOD_MS); // Lento cuando MSC inactivo
    }
  }
}
void mscStatusTask(void *pvParameters) {
  while(1) {
    if (usbReady) {
      // Monitorear uso de memoria cada 5 segundos
      USBSerial.printf("Memoria - Libre: %d, PSRAM libre: %d\n", 
                   ESP.getFreeHeap(), ESP.getFreePsram());
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}


void serialTask(void *pvParameters) {
  while(!USBSerial);
  USBSerial.printf("usb task\n");
   //psram();
  while(1) {
    if(usbReady && USBSerial.available()) {
      USBSerial.write(USBSerial.read()); // MÁS RÁPIDO
    }
    yield();
  }
}



void psram(){
  if (psramFound()) {
        USBSerial.println("PSRAM encontrada y habilitada.");
      // Mostrar el tamaño total de la PSRAM
      size_t psramSize = ESP.getPsramSize();
      USBSerial.printf("Tamaño total de PSRAM: %d bytes\n", psramSize);

      // Probar la PSRAM con una asignación dinámica
      const int bufferSize = 1024 * 1024 * 1.5; // 1 MB
      if (bufferSize > psramSize) {
        USBSerial.println("El tamaño del búfer solicitado excede la PSRAM disponible.");
        return;
      }

      // Asignar memoria en la PSRAM
      uint8_t *psramBuffer = (uint8_t *)ps_malloc(bufferSize);
      if (psramBuffer == nullptr) {
        USBSerial.println("Fallo al asignar memoria en la PSRAM.");
        return;
      }

      USBSerial.printf("Memoria asignada correctamente en la PSRAM: %d bytes\n", bufferSize);

      // Escribir datos en la PSRAM
      for (int i = 0; i < bufferSize; i++) {
        psramBuffer[i] = i % 256; // Escribir valores de 0 a 255 repetidamente
      }
      USBSerial.println("Datos escritos en la PSRAM.");

      // Leer y verificar los datos desde la PSRAM
      bool testPassed = true;
      for (int i = 0; i < bufferSize; i++) {
        if (psramBuffer[i] != (i % 256)) {
          USBSerial.printf("Error en la posición %d: esperado %d, obtenido %d\n", i, i % 256, psramBuffer[i]);
          testPassed = false;
          break;
        }
      }

      if (testPassed) {
        USBSerial.println("Prueba de PSRAM exitosa: todos los datos coinciden.");
      } else {
        USBSerial.println("Prueba de PSRAM fallida.");
      }

      // Liberar la memoria asignada
      free(psramBuffer);
      USBSerial.println("Memoria liberada.");
  } else {
    USBSerial.println("PSRAM no encontrada o no habilitada.");
    while(1);
  }
}
