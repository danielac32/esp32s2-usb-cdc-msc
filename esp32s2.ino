 
#include "USB.h"
#include "USBMSC.h"
#include "USBCDC.h"
 
#include "test.h"
#include "disk.h"
#include "fat_filelib.h"
 #include <esp_partition.h>
 


USBCDC USBSerial;
USBMSC MSC;
const int ledPin =  15;
bool usbReady = false;
bool mscReady = false;


 

// Variables para la partición
const esp_partition_t* fat_partition = NULL;
static const char* PARTITION_LABEL = "fat32";
static const char* PARTITION_TYPE = "data";
static const char* PARTITION_SUBTYPE = "fat";



extern "C" {
   // uint8_t* mscStorage = NULL;
    unsigned int MSC_SECTOR_SIZE = 512;
    unsigned int MSC_SECTOR_COUNT = 0;
    int flash_read_sector(unsigned int sector, unsigned char* buffer, unsigned int size);
    int flash_write_sector(unsigned int sector, unsigned char* buffer, unsigned int size);
    void flash_commit(void);

}
 uint8_t SPI_FLASH_BUF[4096];

 
bool init_fat_partition() {
    USBSerial.println("Buscando mejor partición para FAT32...");
    
    // Listar todas las particiones disponibles
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    const esp_partition_t* best_partition = NULL;
    size_t max_size = 0;
    
    USBSerial.println("Particiones disponibles:");
    while (it != NULL) {
        const esp_partition_t* p = esp_partition_get(it);
        USBSerial.printf(" - %s: tipo=%d, subtipo=%d, tamaño=%d bytes (%.2f MB)\n", 
                       p->label, p->type, p->subtype, p->size, p->size / (1024.0 * 1024.0));
        
        if (p->type == ESP_PARTITION_TYPE_DATA && p->size > max_size) {
            best_partition = p;
            max_size = p->size;
        }
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    
    if (best_partition != NULL) {
        fat_partition = best_partition;
        
        // Calcular sectores lógicos de 512 bytes
        // Cada bloque físico de 4KB contiene 8 sectores lógicos
        uint32_t physical_blocks = fat_partition->size / 4096;
        MSC_SECTOR_COUNT = physical_blocks * 8;  // 8 sectores de 512 por bloque
        
        USBSerial.printf("\n✅ Usando partición: %s\n", fat_partition->label);
        USBSerial.printf("   - Tamaño físico: %d bytes (%.2f MB)\n", fat_partition->size, fat_partition->size / (1024.0 * 1024.0));
        USBSerial.printf("   - Bloques físicos de 4KB: %d\n", physical_blocks);
        USBSerial.printf("   - Sectores lógicos de 512: %d\n", MSC_SECTOR_COUNT);
        USBSerial.printf("   - Espacio FAT32 usable: %.2f MB\n", (MSC_SECTOR_COUNT * 512) / (1024.0 * 1024.0));
        
        return true;
    }
    
    USBSerial.println("❌ ERROR: No se encontró partición de datos adecuada");
    return false;
}


// SPI_Flash_Read equivalente para flash interna
void SPI_Flash_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead) {
    if (fat_partition == NULL) {
        memset(pBuffer, 0, NumByteToRead);
        return;
    }
    
    esp_err_t err = esp_partition_read(fat_partition, ReadAddr, pBuffer, NumByteToRead);
    if (err != ESP_OK) {
        memset(pBuffer, 0, NumByteToRead);
    }
}

// SPI_Flash_Write_NoCheck equivalente para flash interna
void SPI_Flash_Write_NoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) {
    if (fat_partition == NULL) return;
    
    // Para flash interna, necesitamos borrar antes de escribir
    uint32_t block_size = 4096;
    uint32_t block_address = (WriteAddr / block_size) * block_size;
    
    // Borrar el bloque completo
    esp_partition_erase_range(fat_partition, block_address, block_size);
    
    // Escribir los datos
    esp_partition_write(fat_partition, WriteAddr, pBuffer, NumByteToWrite);
}

// SPI_Flash_Erase_Sector equivalente para flash interna
void SPI_Flash_Erase_Sector(uint32_t sector_num) {
    if (fat_partition == NULL) return;
    
    uint32_t address = sector_num * 4096;
    esp_partition_erase_range(fat_partition, address, 4096);
}

// SPI_Flash_Write equivalente para flash interna (igual que tu implementación)
void SPI_Flash_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) { 
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;    
    uint16_t i;    

    secpos = WriteAddr / 4096; // Dirección del sector
    secoff = WriteAddr % 4096; // Offset dentro del sector
    secremain = 4096 - secoff; // Espacio restante en el sector

    if (NumByteToWrite <= secremain) secremain = NumByteToWrite;

    while(1) {   
        // Leer el sector completo actual
        SPI_Flash_Read(SPI_FLASH_BUF, secpos * 4096, 4096);
        
        // Verificar si necesita borrado
        for(i = 0; i < secremain; i++) {
            if(SPI_FLASH_BUF[secoff + i] != 0xFF) break;
        }
        
        if(i < secremain) {
            // Necesita borrado
            SPI_Flash_Erase_Sector(secpos);
            for(i = 0; i < secremain; i++) {
                SPI_FLASH_BUF[i + secoff] = pBuffer[i];   
            }
            SPI_Flash_Write_NoCheck(SPI_FLASH_BUF, secpos * 4096, 4096);
        } else {
            // Ya está borrado, escribir directamente
            SPI_Flash_Write_NoCheck(pBuffer, WriteAddr, secremain);
        }
        
        if(NumByteToWrite == secremain) break;
        
        // Continuar con el siguiente sector
        secpos++;
        secoff = 0;
        pBuffer += secremain;
        WriteAddr += secremain;
        NumByteToWrite -= secremain;
        if(NumByteToWrite > 4096) secremain = 4096;
        else secremain = NumByteToWrite;
    }
}

// disk_read equivalente para flash interna
static unsigned char disk_read(uint8_t *rxbuf, uint32_t sector, uint32_t count) {
    for(; count > 0; count--) {
        SPI_Flash_Read(rxbuf, sector * 512, 512);
        sector++;
        rxbuf += 512;
    }
    return 0;
}

// disk_write equivalente para flash interna  
static unsigned char disk_write(const uint8_t *txbuf, uint32_t sector, uint32_t count) {
    for(; count > 0; count--) {                       
        SPI_Flash_Write((uint8_t*)txbuf, sector * 512, 512);
        sector++;
        txbuf += 512;
    }
    return 0;
}

// Funciones que llamará disk.c
int flash_read_sector(unsigned int sector, unsigned char* buffer, unsigned int size) {
    disk_read(buffer, sector, 1);
    return size;
}

int flash_write_sector(unsigned int sector, unsigned char* buffer, unsigned int size) {
    disk_write(buffer, sector, 1);
    return size;
}

 
void flash_commit(void) {
    // La flash se escribe inmediatamente
}




int fat_printf(const char *format, ...) {
    char buffer[100];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0 && USBSerial) {
        USBSerial.print(buffer);
    }
    
    return len;
}

 


void erase_partition() {
    USBSerial.println("🧹 Borrando partición completa...");
    
    if (fat_partition == NULL) {
        USBSerial.println("❌ ERROR: Partición no inicializada para borrado");
        return;
    }
    
    // Borrar toda la partición
    esp_err_t err = esp_partition_erase_range(fat_partition, 0, fat_partition->size);
    if (err == ESP_OK) {
        USBSerial.println("✅ Partición borrada completamente");
    } else {
        USBSerial.printf("❌ ERROR borrando partición: %d\n", err);
    }
}

int initFat32() {

  if (!init_fat_partition()) {
        return -1;
    }
 
    
    uint32_t size = sd_init();  // llama a tu disk.c
    fl_init();

    if (fl_attach_media(sd_readsector, sd_writesector) != FAT_INIT_OK) {
        fat_printf("ERROR: No se pudo montar FAT32\n");
        // Intentar formatear
        if (fl_format(size, "ESP32_DISK")) {
            fat_printf("✅ Formateo exitoso. Reintentando...\n");
            return initFat32(); // recursivo (mejor que goto)
        } else {
            fat_printf("❌ Falló el formateo\n");
            return -1;
        }
    }

    fat_printf("✅ FAT32 montado: %u sectores\n", MSC_SECTOR_COUNT);
    fl_listdirectory("/");
    return 0;
}







void setup() {
  pinMode(ledPin,OUTPUT);
  Serial.begin(115200);
 //disableWDT();
  Serial.setDebugOutput(true);

  /*if (psramFound()) {
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
  }*/


  /*uint32_t available_sectors = calculateAvailableFlash();
  MSC_SECTOR_COUNT = available_sectors;
  
  // Inicializar EEPROM con el tamaño calculado
  EEPROM.begin(MSC_SECTOR_COUNT * MSC_SECTOR_SIZE);
  mscStorage = EEPROM.getDataPtr();
  
  // Inicializar con ceros si es la primera vez
  memset(mscStorage, 0, MSC_SECTOR_COUNT * MSC_SECTOR_SIZE);*/

 
    
  
  USBSerial.begin();
  /*MSC.vendorID("ESP32");
  MSC.productID("USB_MSC_RAM");
  MSC.productRevision("1.0");
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.onStartStop(onStartStop);
  MSC.mediaPresent(true);
  MSC.begin(MSC_SECTOR_COUNT, MSC_SECTOR_SIZE);*/

  USB.onEvent(usbEventCallback);
  //MSC.onEvent(mscEventCallback);
  USB.begin();
  

   
  delay(100);
 
  
  xTaskCreate(blinkTask, "BlinkTask", 4096*2, NULL, 1, NULL);
  xTaskCreate(serialTask, "SerialTask", 8192, NULL, 1, NULL);
 // xTaskCreate(mscStatusTask, "MSCStatus", 4096, NULL, 1, NULL);
  
   
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
      vTaskDelay(1000 / portTICK_PERIOD_MS); // Lento cuando MSC inactivo
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
  while (!USBSerial); // Esperar a que el USB esté listo
  USBSerial.println("Iniciando tarea USB...");


     

                    
  // Inicializar el sistema de archivos
  initFat32();


  
  // Verificar si el archivo ya existe
  void* fd = fl_fopen((const char*)"/test2.txt", "r");
  if (fd != NULL) {
    USBSerial.println("✅ ¡El archivo PERSISTIÓ después del reinicio!");
    fl_fclose(fd);

    // Opcional: leer contenido
    fd = fl_fopen((const char*)"/test2.txt", "r");
    char buffer[100];
    int len = fl_fread(buffer, 1, sizeof(buffer) - 1, fd);
    if (len > 0) {
      buffer[len] = '\0';
      USBSerial.printf("Contenido: %s\n", buffer);
    }
    fl_fclose(fd);
  } else {
    USBSerial.println("Archivo no encontrado. Creando nuevo...");
    fd = fl_fopen((const char*)"/test2.txt", "w");
    if (fd) {
      const char* msg = "Hola, persistencia!";
      //fl_fwrite(msg, 1, strlen(msg), fd);
      fl_fputs(msg,fd);
      fl_fclose(fd);
      USBSerial.println("Archivo creado y escrito.");
    } else {
      USBSerial.println("❌ Error al crear el archivo.");
    }
  }

  // Listar directorio
  fl_listdirectory("/");

  // Echo loop
  while (1) {
    if (usbReady && USBSerial.available()) {
      char c= USBSerial.read();
      if(c=='f'){
        if (fl_format(sd_init(), "ESP32_DISK")) {
            fat_printf("✅ Formateo exitoso. Reintentando...\n");
            initFat32(); // recursivo (mejor que goto)
        } else {
            fat_printf("❌ Falló el formateo\n");
        }
      }
      USBSerial.write(c);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

 
