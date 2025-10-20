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

/*
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
    if (WriteAddr >= fat_partition->size) return;
    if (WriteAddr + NumByteToWrite > fat_partition->size) {
        NumByteToWrite = fat_partition->size - WriteAddr;
    }

    const uint32_t block_size = 4096;
    uint32_t block_address = (WriteAddr / block_size) * block_size;
    uint32_t offset_in_block = WriteAddr % block_size;

    uint8_t* block_buffer = (uint8_t*)malloc(block_size);
    if (!block_buffer) {
        // Manejo de error: no se puede asignar
        return;
    }

    // 1. Leer bloque actual
    esp_partition_read(fat_partition, block_address, block_buffer, block_size);
    // 2. Modificar la parte necesaria
    memcpy(block_buffer + offset_in_block, pBuffer, NumByteToWrite);
    // 3. Borrar el bloque
    esp_partition_erase_range(fat_partition, block_address, block_size);
    // 4. Escribir el bloque actualizado
    esp_partition_write(fat_partition, block_address, block_buffer, block_size);

    free(block_buffer); // Liberar memoria
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
}*/


static int32_t onWriteOptimized(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) 
{
    if (partition == NULL) return 0;
    
    static uint8_t sector_buffer[4096];
    size_t addr = Partition->address + (lba * 512) + offset;
    size_t sector_addr = addr - Partition->address;
    size_t sector_base = sector_addr & ~(4095); // Base del sector 4K
    
    // Si la escritura no está alineada o no es del tamaño completo, necesitamos leer-modificar-escribir
    if ((sector_addr % 4096 != 0) || (bufsize != 4096)) {
        // Leer el sector completo
        esp_partition_read(Partition, sector_base, sector_buffer, 4096);
        
        // Copiar los nuevos datos al buffer
        memcpy(sector_buffer + (sector_addr - sector_base), buffer, bufsize);
        
        // Borrar y escribir el sector completo
        esp_partition_erase_range(Partition, sector_base, 4096);
        esp_partition_write(Partition, sector_base, sector_buffer, 4096);
    } else {
        // Escritura alineada de sector completo
        esp_partition_erase_range(Partition, sector_base, 4096);
        esp_partition_write(Partition, sector_base, buffer, bufsize);
    }
    
    return bufsize;
}

// Funciones compatibles con tu interfaz original
void SPI_Flash_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead) 
{
    if (partition == NULL) return;
    
    esp_err_t err = esp_partition_read(Partition, ReadAddr, pBuffer, NumByteToRead);
    if (err != ESP_OK) {
        printf("Error en SPI_Flash_Read: %s\n", esp_err_to_name(err));
    }
}

void SPI_Flash_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite) 
{
    if (partition == NULL) return;
    
    // Para escritura simple, usamos la función optimizada
    onWriteOptimized(WriteAddr / 512, WriteAddr % 512, pBuffer, NumByteToWrite);
}

// Función de borrado de sector
void SPI_Flash_Erase_Sector(uint32_t SectorNum) 
{
    if (partition == NULL) return;
    
    size_t addr = SectorNum * 4096;
    esp_err_t err = esp_partition_erase_range(Partition, addr, 4096);
    if (err != ESP_OK) {
        printf("Error borrando sector: %s\n", esp_err_to_name(err));
    }
}



unsigned char disk_read(uint8_t *rxbuf, uint32_t sector, uint32_t count) {
     
    if (Partition == NULL) return 1;
    
    for(; count > 0; count--) {
        esp_err_t err = esp_partition_read(Partition, 
                                         sector * 512, 
                                         rxbuf, 
                                         512);
        if (err != ESP_OK) {
            printf("Error en disk_read: %s\n", esp_err_to_name(err));
            return 1;
        }
        sector++;
        rxbuf += 512;
    }
    return 0;
}

// disk_write equivalente para flash interna  
unsigned char disk_write(const uint8_t *txbuf, uint32_t sector, uint32_t count) {
     
    if (Partition == NULL) return 1;
    
    for(; count > 0; count--) {
        // Usar la función optimizada para escritura
        if (onWriteOptimized(sector, 0, (uint8_t*)txbuf, 512) != 512) {
            return 1;
        }
        sector++;
        txbuf += 512;
    }
    return 0;
}
 
 
 
