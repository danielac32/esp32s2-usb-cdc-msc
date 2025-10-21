 
#include "USB.h"
#include "USBMSC.h"
#include "USBCDC.h"
 
 
#include <src/fat32/disk.h>
#include <src/fat32/fat_filelib.h>
#include <esp_partition.h>

const esp_partition_t* Partition;
#define BLOCK_SIZE 512

 
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
    //extern unsigned char *rambuf;
    unsigned char diskio_read(uint8_t *rxbuf, uint32_t sector, uint32_t count);
    unsigned char diskio_write(const uint8_t *txbuf, uint32_t sector, uint32_t count);
}
  
 



int fat_printf(const char *format, ...) {
    static char buffer[100];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0 && USBSerial) {
        USBSerial.print(buffer);
    }
    
    return len;
}

 
 
int initFat32() {
    if (!init_fat_partition()) {
        return -1;
    }
    //uint32_t size = sd_init();  // llama a tu disk.c
    fl_init();

    if (fl_attach_media(sd_readsector, sd_writesector) != FAT_INIT_OK) {
        fat_printf("ERROR: No se pudo montar FAT32\n");
        // Intentar formatear
        if (fl_format(Partition->size/1, "ESP32_DISK")) {
            fat_printf("✅ Formateo exitoso. Reintentando...\n");
            return initFat32(); // recursivo (mejor que goto)
        } else {
            fat_printf("❌ Falló el formateo\n");
            return -1;
        }
        return -1;
    }
    fat_printf("%d %d %d\n",Partition->size/BLOCK_SIZE, BLOCK_SIZE,Partition->size/BLOCK_SIZE* BLOCK_SIZE);
    fat_printf("✅ FAT32 montado: %u sectores\n", MSC_SECTOR_COUNT);
    fl_listdirectory("/");

    return 0;
}


const uint8_t suma_machine_code[] = {
    0x03, 0x20, 0x82, 0x00,  // add a2, a2, a3  (a2 = a2 + a3)
    0x02, 0x00, 0x06, 0x00,  // mov a0, a2      (a0 = a2 - valor de retorno)
    0x00, 0x00, 0x00, 0x00   // ret (nop para alineamiento)
};

// Versión alternativa más robusta
const uint8_t suma_machine_code_alt[] = {
    // Prologo (opcional para función simple)
    0x36, 0x41,              // entry a1, 32 (reserva espacio stack)
    
    // Cuerpo de la función: a + b
    0x03, 0x20, 0x82, 0x00,  // add a2, a2, a3
    
    // Preparar valor de retorno
    0x02, 0x00, 0x06, 0x00,  // mov a0, a2
    
    // Epilogo
    0x30, 0x01,              // retw (return from window)
    0x00, 0x00, 0x00, 0x00   // padding
};




void setup() {
  pinMode(ledPin,OUTPUT);
  Serial.begin(115200);
 
  bool mode=true;
  Partition = partition();
  USBSerial.begin();
  USB.begin();
  while (!USBSerial); 
  USBSerial.println("\n🚀 Iniciando sistema FAT32 con USB MSC...");

  initFat32();


 /* if (!psramFound()) {
      return;
  }
  //rambuf = (unsigned char*)ps_malloc(200000);
  
  */



   

  
    
  MSC_SECTOR_COUNT = Partition->size/BLOCK_SIZE;
  MSC.vendorID("ESP32");
  MSC.productID("USB_MSC_FLASH");
  MSC.productRevision("1.0");
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.onStartStop(onStartStop);
  MSC.mediaPresent(true);
  MSC.isWritable(true);
  MSC.begin(Partition->size/BLOCK_SIZE, BLOCK_SIZE);
  USB.begin();
  

 /*
  if(mode){
    USBSerial.begin();
    USB.onEvent(usbEventCallback);
    USB.begin();
    xTaskCreate(serialTask, "SerialTask", 8192, NULL, 1, NULL);
  }else{
    MSC.vendorID("ESP32");
    MSC.productID("USB_MSC_FLASH");
    MSC.productRevision("1.0");
    MSC.onRead(onRead);
    MSC.onWrite(onWrite);
    MSC.onStartStop(onStartStop);
    MSC.mediaPresent(true);
    MSC.isWritable(true);
    MSC.begin(Partition->size/BLOCK_SIZE, BLOCK_SIZE);
    USB.begin();
  }
  
  xTaskCreate(blinkTask, "BlinkTask", 4096*2, NULL, 1, NULL);
 
  */
   xTaskCreate(blinkTask, "BlinkTask", 4096*2, NULL, 1, NULL);
}
int count=0;
void loop() {
  USBSerial.printf(" %d\n",count++);
  
  delay(1000);

}
