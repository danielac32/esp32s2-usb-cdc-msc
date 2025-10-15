 
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




extern "C" {
   // uint8_t* mscStorage = NULL;
    unsigned int MSC_SECTOR_SIZE = 512;
    unsigned int MSC_SECTOR_COUNT = 0;
    int flash_read_sector(unsigned int sector, unsigned char* buffer, unsigned int size);
    int flash_write_sector(unsigned int sector, unsigned char* buffer, unsigned int size);
    void flash_commit(void);

    unsigned char disk_read(uint8_t *rxbuf, uint32_t sector, uint32_t count);
    unsigned char disk_write(const uint8_t *txbuf, uint32_t sector, uint32_t count);
}
 uint8_t block_buffer[4096];//uint8_t SPI_FLASH_BUF[4096];

 
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


void SPI_Flash_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead) {
    if (fat_partition == NULL) {
        memset(pBuffer, 0, NumByteToRead);
        return;
    }
    
    // VERIFICAR que la dirección esté dentro de los límites
    if (ReadAddr >= fat_partition->size) {
        memset(pBuffer, 0, NumByteToRead);
        return;
    }
    
    // Ajustar tamaño si excede los límites
    if (ReadAddr + NumByteToRead > fat_partition->size) {
        NumByteToRead = fat_partition->size - ReadAddr;
    }
    
    esp_partition_read(fat_partition, ReadAddr, pBuffer, NumByteToRead);
}

void SPI_Flash_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) { 
    if (fat_partition == NULL) return;
    
    // VERIFICAR que la dirección esté dentro de los límites
    if (WriteAddr >= fat_partition->size) return;
    
    // Ajustar tamaño si excede los límites
    if (WriteAddr + NumByteToWrite > fat_partition->size) {
        NumByteToWrite = fat_partition->size - WriteAddr;
    }
    
    // Para flash interna: siempre borrar el bloque completo antes de escribir
    uint32_t block_size = 4096;
    uint32_t block_address = (WriteAddr / block_size) * block_size;
    
    // Buffer para el bloque completo
    //uint8_t block_buffer[4096];
    
    // 1. Leer el bloque completo actual (para preservar otros datos)
    esp_partition_read(fat_partition, block_address, block_buffer, block_size);
    
    // 2. Actualizar solo la parte que vamos a escribir
    uint32_t offset_in_block = WriteAddr % block_size;
    memcpy(block_buffer + offset_in_block, pBuffer, NumByteToWrite);
    
    // 3. Borrar el bloque completo
    esp_partition_erase_range(fat_partition, block_address, block_size);
    
    // 4. Escribir el bloque completo actualizado
    esp_partition_write(fat_partition, block_address, block_buffer, block_size);
}


void SPI_Flash_Write_NoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) {
    if (fat_partition == NULL) return;
    esp_partition_write(fat_partition, WriteAddr, pBuffer, NumByteToWrite);
}

void SPI_Flash_Erase_Sector(uint32_t sector_num) {
    if (fat_partition == NULL) return;
    esp_partition_erase_range(fat_partition, sector_num * 4096, 4096);
}







// disk_read equivalente para flash interna
unsigned char disk_read(uint8_t *rxbuf, uint32_t sector, uint32_t count) {
    for(; count > 0; count--) {
        SPI_Flash_Read(rxbuf, sector * 512, 512);
        sector++;
        rxbuf += 512;
    }
    return 0;
}

// disk_write equivalente para flash interna  
unsigned char disk_write(const uint8_t *txbuf, uint32_t sector, uint32_t count) {
    for(; count > 0; count--) {                       
        SPI_Flash_Write((uint8_t*)txbuf, sector * 512, 512);
        sector++;
        txbuf += 512;
    }
    return 0;
}

// Funciones que llamará disk.c
int flash_read_sector(unsigned int sector, unsigned char* buffer, unsigned int size) {
    
    return 1;
}

int flash_write_sector(unsigned int sector, unsigned char* buffer, unsigned int size) {
   
    return 1;
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
