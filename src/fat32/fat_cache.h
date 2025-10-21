#ifndef __FAT_CACHE_H__
#define __FAT_CACHE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "integer.h"
#include "fat_filelib.h"

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
int fatfs_cache_init(struct fatfs *fs, FL_FILE *file);
int fatfs_cache_get_next_cluster(struct fatfs *fs, FL_FILE *file, uint32 clusterIdx, uint32 *pNextCluster);
int fatfs_cache_set_next_cluster(struct fatfs *fs, FL_FILE *file, uint32 clusterIdx, uint32 nextCluster);

#ifdef __cplusplus
}
#endif

#endif // TEST_H