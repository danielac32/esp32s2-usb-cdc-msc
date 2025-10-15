 
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

   

 
    
  
  USBSerial.begin();
  USB.onEvent(usbEventCallback);
  MSC.vendorID("ESP32");
  MSC.productID("USB_MSC_FLASH");
  MSC.productRevision("1.0");
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.onStartStop(onStartStop);
  MSC.mediaPresent(true);
  MSC.isWritable(true);
  MSC.begin(MSC_SECTOR_COUNT, MSC_SECTOR_SIZE);

  
  USB.begin();
  


   
  delay(100);
 
  
  xTaskCreate(blinkTask, "BlinkTask", 4096*2, NULL, 1, NULL);
 // xTaskCreate(serialTask, "SerialTask", 8192, NULL, 1, NULL);
 
  
   
}

void loop() {
  delay(1000);

}
