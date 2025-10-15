#include <disk.h>
#include <stdio.h>
#include <string.h>

//extern unsigned char* mscStorage;
extern unsigned int MSC_SECTOR_SIZE;
extern unsigned int MSC_SECTOR_COUNT;
extern int fat_printf(const char *format, ...);
extern int flash_read_sector(unsigned int sector, unsigned char* buffer, unsigned int size);
extern int flash_write_sector(unsigned int sector, unsigned char* buffer, unsigned int size);

extern unsigned char disk_read(uint8_t *rxbuf, uint32_t sector, uint32_t count);
extern unsigned char disk_write(const uint8_t *txbuf, uint32_t sector, uint32_t count);
extern void flash_commit(void);

int sd_init(void) {
    fat_printf("Storage size: %d sectores, %d bytes\n", MSC_SECTOR_COUNT, MSC_SECTOR_SIZE * MSC_SECTOR_COUNT);
    return MSC_SECTOR_SIZE * MSC_SECTOR_COUNT;
}

int sd_readsector(unsigned int start_block, unsigned char *buffer, unsigned int sector_count) {
    disk_read(buffer, start_block, sector_count);
    return 1;
}

int sd_writesector(unsigned int start_block, unsigned char *buffer, unsigned int sector_count) {
    disk_write(buffer, start_block, sector_count);
    return 1;
}