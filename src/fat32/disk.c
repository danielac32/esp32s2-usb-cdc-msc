#include "disk.h"
#include <stdio.h>
#include <string.h>

//extern unsigned char* mscStorage;
extern unsigned int MSC_SECTOR_SIZE;
extern unsigned int MSC_SECTOR_COUNT;
extern int fat_printf(const char *format, ...);
 
extern unsigned char diskio_read(uint8_t *rxbuf, uint32_t sector, uint32_t count);
extern unsigned char diskio_write(const uint8_t *txbuf, uint32_t sector, uint32_t count);
extern void flash_commit(void);

int sd_init(void) {
    fat_printf("Storage size: %d sectores, %d bytes\n", MSC_SECTOR_COUNT, MSC_SECTOR_SIZE * MSC_SECTOR_COUNT);
    return MSC_SECTOR_SIZE * MSC_SECTOR_COUNT;
}

int sd_readsector(unsigned int start_block, unsigned char *buffer, unsigned int sector_count) {
    diskio_read(buffer, start_block, sector_count);
    return 1;
}

int sd_writesector(unsigned int start_block, unsigned char *buffer, unsigned int sector_count) {
   diskio_write(buffer, start_block, sector_count);
    return 1;
}