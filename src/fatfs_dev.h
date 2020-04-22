/* Copyright 2011-2016 Tyler Gilbert; All Rights Reserved
 * 
 * 
 */


#ifndef CAFS_DEV_H_
#define CAFS_DEV_H_

#include <pthread.h>
#include <sos/dev/drive.h>
#include "integer.h"


#define FATFS_CONFIG(cfg) ((fatfs_config_t*)cfg)
#define FATFS_STATE(cfg) ((fatfs_state_t*)(((fatfs_config_t*)cfg)->drive.state))
#define FATFS_DRIVE(cfg) &(((fatfs_config_t*)cfg)->drive)
#define FATFS_DRIVE_MUTEX(cfg) &(((fatfs_config_t*)cfg)->drive.state->mutex)

int fatfs_dev_cfg_volume(const void * cfg);

int fatfs_dev_open(BYTE pdrv);
int fatfs_dev_write(BYTE pdrv, int loc, const void * buf, int nbyte);
int fatfs_dev_read(BYTE pdrv, int loc, void * buf, int nbyte);
int fatfs_dev_close(BYTE pdrv);

int fatfs_dev_status(BYTE pdrv);

int fatfs_dev_ioctl(BYTE pdrv, int request, void * ctl);
int fatfs_dev_getinfo(BYTE pdrv, drive_info_t * info);

int fatfs_dev_waitbusy(BYTE pdrv);
int fatfs_dev_eraseblocks(BYTE pdrv, int start, int end);

int sysfs_access(int file_mode, int file_uid, int file_gid, int amode);
//int cl_sys_geteuid();
//int cl_sys_getegid();

void fatfs_dev_setdelay_mutex(pthread_mutex_t * mutex);

#endif /* CAFS_DEV_H_ */
