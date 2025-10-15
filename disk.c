#include <disk.h>
#include <stdio.h>
#include <string.h>

//extern unsigned char* mscStorage;
extern unsigned int MSC_SECTOR_SIZE;
extern unsigned int MSC_SECTOR_COUNT;
extern int fat_printf(const char *format, ...);
extern int flash_read_sector(unsigned int sector, unsigned char* buffer, unsigned int size);
extern int flash_write_sector(unsigned int sector, unsigned char* buffer, unsigned int size);
extern void flash_commit(void);

int sd_init(void) {
    fat_printf("Storage size: %d sectores, %d bytes\n", MSC_SECTOR_COUNT, MSC_SECTOR_SIZE * MSC_SECTOR_COUNT);
    return MSC_SECTOR_SIZE * MSC_SECTOR_COUNT;
}

int sd_readsector(unsigned int start_block, unsigned char *buffer, unsigned int sector_count) {
    for (unsigned int i = 0; i < sector_count; i++) {
        int result = flash_read_sector(start_block + i, buffer + (i * MSC_SECTOR_SIZE), MSC_SECTOR_SIZE);
        if (result != MSC_SECTOR_SIZE) {
            return i;
        }
    }
    return sector_count;
}

int sd_writesector(unsigned int start_block, unsigned char *buffer, unsigned int sector_count) {
    for (unsigned int i = 0; i < sector_count; i++) {
        int result = flash_write_sector(start_block + i, buffer + (i * MSC_SECTOR_SIZE), MSC_SECTOR_SIZE);
        if (result != MSC_SECTOR_SIZE) {
            return i;
        }
    }
    flash_commit();
    return sector_count;
}