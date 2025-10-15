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
