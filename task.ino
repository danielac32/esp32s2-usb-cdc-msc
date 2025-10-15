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
    while (!USBSerial);
    USBSerial.println("🚀 Iniciando sistema FAT32...");

    // Inicializar el sistema de archivos
    if (initFat32() == 0) {
        USBSerial.println("✅ FAT32 inicializado correctamente");
        
        // Ejecutar pruebas
        //testFileSystem();
        
        
        
        // Probar persistencia (simulando reinicio)
       // testPersistencia();
        
    } else {
        USBSerial.println("❌ Error inicializando FAT32");
    }

    // Loop principal con comandos
    while (1) {
        if (usbReady && USBSerial.available()) {
            char c = USBSerial.read();
            
            switch(c) {
                case 'f': // Formatear
                    USBSerial.println("🔄 Formateando...");
                    if (fl_format(sd_init(), "ESP32_DISK")) {
                        USBSerial.println("✅ Formateo exitoso");
                        initFat32();
                    } else {
                        USBSerial.println("❌ Falló el formateo");
                    }
                    break;
                    
                case 't': // Ejecutar pruebas
                    //testFileSystem();
                    testContenidoArchivos();
                    break;
                    
                case 'p': // Probar persistencia
                    testPersistencia();
                    break;
                    
                case 'l': // Listar directorio
                    USBSerial.println("📁 Directorio raíz:");
                    fl_listdirectory("/");
                    break;
                    
                case 'c': // Crear archivo de test
                    {
                        void* fd = fl_fopen("/comando.txt", "w");
                        if (fd) {
                            fl_fwrite("Archivo creado por comando", 1, 26, fd);
                            fl_fclose(fd);
                            USBSerial.println("✅ Archivo creado: /comando.txt");
                        }
                    }
                    break;
                    
                case 'd': // Mostrar información del disco
                    USBSerial.printf("💾 Sectores: %d, Tamaño: %.2f MB\n", 
                                   MSC_SECTOR_COUNT, 
                                   (MSC_SECTOR_COUNT * 512) / (1024.0 * 1024.0));
                    break;
                    
                default:
                    USBSerial.write(c);
                    break;
            }
            
            // Mostrar menú de comandos
            if (c == 'h' || c == '?') {
                USBSerial.println("\n📋 Comandos disponibles:");
                USBSerial.println("f - Formatear disco");
                USBSerial.println("t - Ejecutar pruebas de archivos");
                USBSerial.println("p - Probar persistencia");
                USBSerial.println("l - Listar directorio");
                USBSerial.println("c - Crear archivo de test");
                USBSerial.println("d - Mostrar información del disco");
                USBSerial.println("h - Mostrar esta ayuda");
            }
        }
    }
}
 
