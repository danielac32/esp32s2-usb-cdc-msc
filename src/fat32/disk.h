
/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2014          /
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED
#ifdef __cplusplus
extern "C" {
#endif
	
/*---------------------------------------*/
/* Prototypes for disk control functions */

extern unsigned char* mscStorage;
extern unsigned int MSC_SECTOR_SIZE;
extern unsigned int MSC_SECTOR_COUNT;

int sd_init(void);
int sd_readsector(unsigned int start_block, unsigned char *buffer, unsigned int sector_count);
int sd_writesector(unsigned int start_block, unsigned char *buffer, unsigned int sector_count);
#ifdef __cplusplus
}
#endif

#endif // TEST_H