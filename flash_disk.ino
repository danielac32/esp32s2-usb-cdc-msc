bool init_fat_partition() {
    fat_printf("Buscando mejor partición para FAT32...\n");
    
    // Listar todas las particiones disponibles
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    const esp_partition_t* best_partition = NULL;
    size_t max_size = 0;
    
    fat_printf("Particiones disponibles:\n");
    while (it != NULL) {
        const esp_partition_t* p = esp_partition_get(it);
        fat_printf(" - %s: tipo=%d, subtipo=%d, tamaño=%d bytes (%.2f MB)\n", 
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
        
        fat_printf("\n✅ Usando partición: %s\n", fat_partition->label);
        fat_printf("   - Tamaño físico: %d bytes (%.2f MB)\n", fat_partition->size, fat_partition->size / (1024.0 * 1024.0));
        fat_printf("   - Bloques físicos de 4KB: %d\n", physical_blocks);
        fat_printf("   - Sectores lógicos de 512: %d\n", MSC_SECTOR_COUNT);
        fat_printf("   - Espacio FAT32 usable: %.2f MB\n", (MSC_SECTOR_COUNT * 512) / (1024.0 * 1024.0));
        
        return true;
    }
    
    fat_printf("❌ ERROR: No se encontró partición de datos adecuada\n");
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
 
 
 
