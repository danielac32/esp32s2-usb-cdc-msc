uint32_t calculateAvailableFlash() {
    USBSerial.println("\n=== FLASH DISPONIBLE PARA DATOS ===");
    
    uint32_t total_flash = ESP.getFlashChipSize();
    uint32_t used_by_program = ESP.getSketchSize();
    
    // Estimación más precisa del espacio reservado
    // - Bootloader: ~32KB
    // - Partition table: ~16KB  
    // - NVS: ~24KB
    // - OTA data: ~8KB
    // - Otros: ~32KB
    uint32_t system_reserved = 32*1024 + 16*1024 + 24*1024 + 8*1024 + 32*1024; // ~112KB
    
    uint32_t available_for_data = total_flash - used_by_program - system_reserved;
    
    // Asegurarnos de no exceder el espacio disponible
    if (available_for_data > total_flash) {
        available_for_data = 0;
    }
    
    USBSerial.printf("Flash total: %d bytes (%.2f MB)\n", 
                    total_flash, total_flash / (1024.0 * 1024.0));
    USBSerial.printf("Usado por programa: %d bytes (%.2f KB)\n", 
                    used_by_program, used_by_program / 1024.0);
    USBSerial.printf("Reservado sistema: %d bytes (%.2f KB)\n", 
                    system_reserved, system_reserved / 1024.0);
    USBSerial.printf("DISPONIBLE para datos: %d bytes (%.2f MB)\n", 
                    available_for_data, available_for_data / (1024.0 * 1024.0));
    
    // Calcular sectores FAT32
    uint32_t available_sectors = available_for_data / 512;
    USBSerial.printf("Sectores FAT32 disponibles (512 bytes): %d\n", available_sectors);
    
    // Recomendación de uso seguro (80% del disponible)
    uint32_t safe_sectors = available_sectors * 0.8;
    USBSerial.printf("Uso recomendado (80%%): %d sectores (%.2f MB)\n", 
                    safe_sectors, (safe_sectors * 512) / (1024.0 * 1024.0));
    
    return safe_sectors;
}
 


 
void showFlashInfo() {
    USBSerial.println("=== INFORMACIÓN DE FLASH ===");
    
    // Capacidad total de flash
    uint32_t flash_size = ESP.getFlashChipSize();
    USBSerial.printf("Tamaño total de flash: %d bytes (%.2f MB)\n", 
                    flash_size, flash_size / (1024.0 * 1024.0));
    
    // Velocidad de la flash
    uint32_t flash_speed = ESP.getFlashChipSpeed();
    USBSerial.printf("Velocidad de flash: %d Hz (%.1f MHz)\n", 
                    flash_speed, flash_speed / 1000000.0);
    
    // Modo de flash
    FlashMode_t flash_mode = ESP.getFlashChipMode();
    const char* mode_str = "Desconocido";
    switch(flash_mode) {
        case FM_QIO:  mode_str = "QIO"; break;
        case FM_QOUT: mode_str = "QOUT"; break;
        case FM_DIO:  mode_str = "DIO"; break;
        case FM_DOUT: mode_str = "DOUT"; break;
    }
    USBSerial.printf("Modo de flash: %s\n", mode_str);
    
    // Flash usado por el programa
    uint32_t sketch_size = ESP.getSketchSize();
    uint32_t free_sketch_space = ESP.getFreeSketchSpace();
    USBSerial.printf("Tamaño del programa: %d bytes (%.2f KB)\n", 
                    sketch_size, sketch_size / 1024.0);
    USBSerial.printf("Espacio libre para programa: %d bytes (%.2f KB)\n", 
                    free_sketch_space, free_sketch_space / 1024.0);
    
    // Información de la partición actual
    USBSerial.printf("Chip ID: %08X\n", ESP.getEfuseMac());
    
    
}



























void testFileSystem() {
    USBSerial.println("\n🧪 === PRUEBA COMPLETA DEL SISTEMA DE ARCHIVOS ===");
    
    // 1. Crear y escribir un archivo
    USBSerial.println("1. Creando archivo de prueba...");
    void* fd = fl_fopen("/prueba.txt", "w");
    if (fd) {
        const char* contenido = "Este es un texto de prueba para verificar la persistencia!";
        int escritos = fl_fwrite(contenido, 1, strlen(contenido), fd);
        fl_fclose(fd);
        USBSerial.printf("   ✅ Archivo creado, %d bytes escritos\n", escritos);
    } else {
        USBSerial.println("   ❌ Error creando archivo");
        return;
    }
    
    // 2. Leer el archivo recién creado
    USBSerial.println("2. Leyendo archivo recién creado...");
    fd = fl_fopen("/prueba.txt", "r");
    if (fd) {
        char buffer[100];
        int leidos = fl_fread(buffer, 1, sizeof(buffer) - 1, fd);
        fl_fclose(fd);
        if (leidos > 0) {
            buffer[leidos] = '\0';
            USBSerial.printf("   ✅ Lectura: %d bytes\n", leidos);
            USBSerial.printf("   📝 Contenido: '%s'\n", buffer);
        } else {
            USBSerial.println("   ❌ No se pudieron leer datos");
        }
    } else {
        USBSerial.println("   ❌ Error abriendo archivo para lectura");
    }
    
    // 3. Verificar existencia con fl_fopen en modo lectura
    USBSerial.println("3. Verificando existencia del archivo...");
    fd = fl_fopen("/prueba.txt", "r");
    if (fd) {
        USBSerial.println("   ✅ Archivo existe");
        fl_fclose(fd);
    } else {
        USBSerial.println("   ❌ Archivo no existe");
    }
    
    // 4. Probar fl_fseek y fl_ftell
    USBSerial.println("4. Probando posicionamiento en archivo...");
    fd = fl_fopen("/prueba.txt", "r");
    if (fd) {
        fl_fseek(fd, 10, 0); // Ir al byte 10
        long pos = fl_ftell(fd);
        USBSerial.printf("   ✅ Posición después de seek(10): %ld\n", pos);
        
        char buffer[50];
        int leidos = fl_fread(buffer, 1, 20, fd);
        if (leidos > 0) {
            buffer[leidos] = '\0';
            USBSerial.printf("   📝 Leyendo desde posición %ld: '%s'\n", pos, buffer);
        }
        fl_fclose(fd);
    }
    
    // 5. Probar fl_feof
    USBSerial.println("5. Probando fin de archivo...");
    fd = fl_fopen("/prueba.txt", "r");
    if (fd) {
        char buffer[200];
        int total_leidos = 0;
        while (!fl_feof(fd)) {
            int leidos = fl_fread(buffer + total_leidos, 1, 50, fd);
            if (leidos <= 0) break;
            total_leidos += leidos;
        }
        buffer[total_leidos] = '\0';
        USBSerial.printf("   ✅ Total leídos: %d bytes\n", total_leidos);
        USBSerial.printf("   📝 Contenido completo: '%s'\n", buffer);
        fl_fclose(fd);
    }
    
    // 6. Crear múltiples archivos
    USBSerial.println("6. Creando múltiples archivos...");
    for (int i = 0; i < 3; i++) {
        char filename[20];
        snprintf(filename, sizeof(filename), "/archivo%d.txt", i);
        fd = fl_fopen(filename, "w");
        if (fd) {
            char contenido[50];
            snprintf(contenido, sizeof(contenido), "Contenido del archivo %d", i);
            fl_fwrite(contenido, 1, strlen(contenido), fd);
            fl_fclose(fd);
            USBSerial.printf("   ✅ Creado: %s\n", filename);
        }
    }
    
    // 7. Listar directorio para ver todos los archivos
    USBSerial.println("7. Listando directorio raíz:");
    fl_listdirectory("/");
    
    // 8. Probar eliminación de archivo
    USBSerial.println("8. Probando eliminación de archivo...");
    if (fl_remove("/archivo1.txt") == 0) {
        USBSerial.println("   ✅ Archivo eliminado correctamente");
    } else {
        USBSerial.println("   ❌ Error eliminando archivo");
    }
    
    USBSerial.println("9. Directorio después de eliminar archivo:");
    fl_listdirectory("/");
    
    USBSerial.println("✅ === PRUEBA COMPLETADA ===\n");
}

void testPersistencia() {
    USBSerial.println("\n💾 === PRUEBA DE PERSISTENCIA ===");
    
    // Verificar si los archivos persisten después de reinicio
    void* fd = fl_fopen("/prueba.txt", "r");
    if (fd) {
        USBSerial.println("✅ ¡Los archivos PERSISTIERON después del reinicio!");
        
        char buffer[100];
        int leidos = fl_fread(buffer, 1, sizeof(buffer) - 1, fd);
        if (leidos > 0) {
            buffer[leidos] = '\0';
            USBSerial.printf("📝 Contenido persistido: '%s'\n", buffer);
        }
        fl_fclose(fd);
    } else {
        USBSerial.println("❌ Los archivos NO persistieron");
    }
    
    // Verificar otros archivos
    int archivos_persistidos = 0;
    for (int i = 0; i < 3; i++) {
        char filename[20];
        snprintf(filename, sizeof(filename), "/archivo%d.txt", i);
        if (fl_fopen(filename, "r") != NULL) {
            archivos_persistidos++;
            fl_fclose(fd);
        }
    }
    USBSerial.printf("📊 Archivos persistidos: %d/3\n", archivos_persistidos);
    
    USBSerial.println("💾 === PRUEBA DE PERSISTENCIA COMPLETADA ===\n");
}






void testContenidoArchivos() {
    USBSerial.println("\n📖 === PRUEBA ESPECÍFICA DE CONTENIDO ===");
    
    // 1. Crear archivo con contenido conocido
    USBSerial.println("1. Creando archivo con contenido específico...");
    const char* contenido_original = "Hola Mundo FAT32 - 1234567890 - ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    void* fd = fl_fopen("/contenido_test.txt", "w");
    if (fd) {
        int escritos = fl_fwrite(contenido_original, 1, strlen(contenido_original), fd);
        fl_fclose(fd);
        USBSerial.printf("   ✅ Escritos: %d bytes\n", escritos);
        USBSerial.printf("   📝 Contenido original: '%s'\n", contenido_original);
    } else {
        USBSerial.println("   ❌ Error creando archivo");
        return;
    }
    
    // 2. Leer inmediatamente y verificar contenido
    USBSerial.println("2. Leyendo contenido inmediatamente...");
    fd = fl_fopen("/contenido_test.txt", "r");
    if (fd) {
        char buffer[100];
        memset(buffer, 0, sizeof(buffer));
        int leidos = fl_fread(buffer, 1, strlen(contenido_original), fd);
        fl_fclose(fd);
        
        USBSerial.printf("   📊 Leídos: %d bytes\n", leidos);
        USBSerial.printf("   📝 Contenido leído: '%s'\n", buffer);
        
        // Verificar si el contenido coincide
        if (leidos == strlen(contenido_original) && 
            memcmp(contenido_original, buffer, leidos) == 0) {
            USBSerial.println("   ✅ ✅ CONTENIDO COINCIDE EXACTAMENTE!");
        } else {
            USBSerial.println("   ❌ ❌ CONTENIDO NO COINCIDE!");
            USBSerial.printf("   Esperado: %d bytes, Leído: %d bytes\n", 
                           strlen(contenido_original), leidos);
            
            // Mostrar diferencias byte por byte
            USBSerial.println("   🔍 Comparación byte a byte:");
            for (int i = 0; i < strlen(contenido_original) || i < leidos; i++) {
                if (i < strlen(contenido_original) && i < leidos) {
                    if (contenido_original[i] != buffer[i]) {
                        USBSerial.printf("      Byte %d: Esperado '%c' (0x%02X), Leído '%c' (0x%02X) ❌\n", 
                                       i, contenido_original[i], contenido_original[i],
                                       buffer[i], buffer[i]);
                    }
                } else if (i < strlen(contenido_original)) {
                    USBSerial.printf("      Byte %d: Esperado '%c' (0x%02X), Leído NADA ❌\n", 
                                   i, contenido_original[i], contenido_original[i]);
                } else {
                    USBSerial.printf("      Byte %d: Esperado NADA, Leído '%c' (0x%02X) ❌\n", 
                                   i, buffer[i], buffer[i]);
                }
            }
        }
    } else {
        USBSerial.println("   ❌ Error abriendo archivo para lectura");
    }
    
    // 3. Probar diferentes métodos de lectura
    USBSerial.println("3. Probando diferentes métodos de lectura...");
    
    // Método 1: fl_fread con tamaño completo
    USBSerial.println("   Método 1: fl_fread completo");
    fd = fl_fopen("/contenido_test.txt", "r");
    if (fd) {
        char buffer[100];
        memset(buffer, 0, sizeof(buffer));
        int leidos = fl_fread(buffer, 1, sizeof(buffer), fd);
        USBSerial.printf("      Leídos: %d bytes -> '%s'\n", leidos, buffer);
        fl_fclose(fd);
    }
    
    // Método 2: fl_fgets
    USBSerial.println("   Método 2: fl_fgets");
    fd = fl_fopen("/contenido_test.txt", "r");
    if (fd) {
        char buffer[100];
        memset(buffer, 0, sizeof(buffer));
        char* resultado = fl_fgets(buffer, sizeof(buffer), fd);
        USBSerial.printf("      fl_fgets: '%s' (resultado: %p)\n", buffer, resultado);
        fl_fclose(fd);
    }
    
    // Método 3: fl_fgetc carácter por carácter
    USBSerial.println("   Método 3: fl_fgetc carácter por carácter");
    fd = fl_fopen("/contenido_test.txt", "r");
    if (fd) {
        USBSerial.print("      Caracteres: '");
        int c;
        int count = 0;
        while ((c = fl_fgetc(fd)) != EOF && count < 50) {
            USBSerial.write(c);
            count++;
        }
        USBSerial.println("'");
        USBSerial.printf("      Total caracteres: %d\n", count);
        fl_fclose(fd);
    }
    
    // 4. Verificar tamaño del archivo
    USBSerial.println("4. Verificando tamaño del archivo...");
    fd = fl_fopen("/contenido_test.txt", "r");
    if (fd) {
        fl_fseek(fd, 0, 2); // Ir al final
        long tamaño = fl_ftell(fd);
        fl_fseek(fd, 0, 0); // Volver al inicio
        USBSerial.printf("   Tamaño reportado: %ld bytes\n", tamaño);
        USBSerial.printf("   Tamaño esperado: %d bytes\n", strlen(contenido_original));
        fl_fclose(fd);
    }
    
    // 5. Probar archivo binario
    USBSerial.println("5. Probando archivo binario...");
    uint8_t datos_binarios[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
                               0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                               0x41, 0x42, 0x43, 0x44}; // ABCD en ASCII
    
    fd = fl_fopen("/binario_test.bin", "w");
    if (fd) {
        int escritos = fl_fwrite(datos_binarios, 1, sizeof(datos_binarios), fd);
        fl_fclose(fd);
        USBSerial.printf("   ✅ Binario escrito: %d bytes\n", escritos);
    }
    
    // Leer archivo binario
    fd = fl_fopen("/binario_test.bin", "r");
    if (fd) {
        uint8_t buffer[50];
        memset(buffer, 0, sizeof(buffer));
        int leidos = fl_fread(buffer, 1, sizeof(datos_binarios), fd);
        USBSerial.printf("   📊 Binario leído: %d bytes\n", leidos);
        
        USBSerial.print("   Bytes leídos: ");
        for (int i = 0; i < leidos; i++) {
            USBSerial.printf("%02X ", buffer[i]);
        }
        USBSerial.println();
        
        // Verificar
        if (memcmp(datos_binarios, buffer, sizeof(datos_binarios)) == 0) {
            USBSerial.println("   ✅ ✅ CONTENIDO BINARIO COINCIDE!");
        } else {
            USBSerial.println("   ❌ ❌ CONTENIDO BINARIO NO COINCIDE!");
        }
        fl_fclose(fd);
    }
    
    USBSerial.println("📖 === PRUEBA DE CONTENIDO COMPLETADA ===\n");
}
