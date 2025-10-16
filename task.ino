void blinkTask(void *pvParameters) {
  while (1) {
    digitalWrite(ledPin, !digitalRead(ledPin));
    
     
      vTaskDelay(100 / portTICK_PERIOD_MS); // Rápido cuando MSC activo
    
    
  }
}
 
 
void serialTask(void *pvParameters) {
    while (!USBSerial);
   // while(!USBSerial.available());
    fat_printf("🚀 Iniciando sistema FAT32...\n");

    // Inicializar el sistema de archivos
    if (initFat32() == 0) {
        fat_printf("✅ FAT32 inicializado correctamente\n");
        
        // Ejecutar pruebas
        testFileSystem();
        
        
        
        // Probar persistencia (simulando reinicio)
        testPersistencia();
        
    } else {
        fat_printf("❌ Error inicializando FAT32\n");
    }

    // Loop principal con comandos
    while (1) {
        if (USBSerial.available()) {
            char c = USBSerial.read();
            
            switch(c) {
                case 'f': // Formatear
                    fat_printf("🔄 Formateando...\n");
                    if (fl_format(sd_init(), "ESP32_DISK")) {
                        fat_printf("✅ Formateo exitoso\n");
                        initFat32();
                    } else {
                        fat_printf("❌ Falló el formateo\n");
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
                    fat_printf("📁 Directorio raíz:\n");
                    fl_listdirectory("/");
                    break;
                    
                case 'c': // Crear archivo de test
                    {
                        void* fd = fl_fopen("/comando.txt", "w");
                        if (fd) {
                            fl_fwrite("Archivo creado por comando", 1, 26, fd);
                            fl_fclose(fd);
                            fat_printf("✅ Archivo creado: /comando.txt\n");
                        }
                    }
                    break;
                    
                case 'd': // Mostrar información del disco
                    fat_printf("💾 Sectores: %d, Tamaño: %.2f MB\n", 
                                   MSC_SECTOR_COUNT, 
                                   (MSC_SECTOR_COUNT * 512) / (1024.0 * 1024.0));
                    break;
                    
                default:
                    fat_printf("%c",c); //USBSerial.write(c);
                    break;
            }
            
            // Mostrar menú de comandos
            if (c == 'h' || c == '?') {
                fat_printf("\n📋 Comandos disponibles:\n");
                fat_printf("f - Formatear disco\n");
                fat_printf("t - Ejecutar pruebas de archivos\n");
                fat_printf("p - Probar persistencia\n");
                fat_printf("l - Listar directorio\n");
                fat_printf("c - Crear archivo de test\n");
                fat_printf("d - Mostrar información del disco\n");
                fat_printf("h - Mostrar esta ayuda\n");
            }
        }
    }
}
 
