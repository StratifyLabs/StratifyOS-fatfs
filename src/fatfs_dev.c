/* Copyright 2011-2016 Tyler Gilbert; All Rights Reserved
 * 
 * 
 */


#include "fatfs.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <sos/dev/drive.h>
#include <sys/lock.h>
#include <time.h>
#include <mcu/core.h>
#include <mcu/debug.h>

#include "ffconf.h"
#include "fatfs.h"
#include "fatfs_dev.h"

#define MAX_RETRIES 10


DWORD get_fattime(){
	return time(0);
}

const void * cfg_table[_VOLUMES];

/*
static void set_delay_mutex(void * args){
	sched_table[ task_get_current() ].signal_delay_mutex = args;
}
 */

int fatfs_dev_cfg_volume(const void * cfg){
	int err;
	const fatfs_config_t * fcfg = (const fatfs_config_t *)cfg;

	err = -1;
	if( fcfg->vol_id < _VOLUMES ){
		cfg_table[fcfg->vol_id] = cfg;
		err = 0;
	}
	return err;
}

void fatfs_dev_setdelay_mutex(pthread_mutex_t * mutex){
	//cortexm_svcall_t(set_delay_mutex, mutex);
}


int fatfs_dev_open(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	drive_attr_t attr;

	cfgp->file->flags = O_RDWR;
	cfgp->file->loc = 0;
	cfgp->file->fs = cfgp->devfs;
	//cfgp->file->handle = NULL;

	if( sysfs_file_open(cfgp->file, cfgp->name, 0) < 0 ){
		return -1;
	}

#if 0
	if( cfgp->file->handle == NULL ){
		ret = cfgp->devfs->open(
				cfgp->devfs->config,
				&(cfgp->file->handle),
				cfgp->name,
				O_RDWR,
				0);

		if( ret < 0 ){
			return -1;
		}
	}
#endif


	attr.o_flags = DRIVE_FLAG_INIT;
	return sysfs_file_ioctl(cfgp->file, I_DRIVE_SETATTR, &attr);
	//return cfgp->devfs->ioctl(cfgp->devfs->cfg, cfgp->open_file->handle, I_DRIVE_SETATTR, &attr);
}

int fatfs_dev_status(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];

	if( cfgp->file->handle != 0 ){
		return 1;
	}

	mcu_debug_user_printf("FATFS: No Dev\n");

	return 0;
}

int fatfs_dev_write(BYTE pdrv, int loc, const void * buf, int nbyte){
	int ret;
	int offset;
	const char * bufp = buf;
	int retries;
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	if ( cfgp->file->fs == NULL ){
        return -1;
	}

	fatfs_dev_waitbusy(pdrv);

	for(offset = 0; offset < nbyte; offset += 512 ){
		retries = 0;
		do {
			cfgp->file->loc = loc;
			ret = sysfs_file_write(cfgp->file, bufp + offset, 512);

			if( ret == 512 ){
				fatfs_dev_waitbusy(pdrv);
			}

			retries++;
		} while( (retries < MAX_RETRIES) && (ret != 512) );
		loc++;

		if( retries > 1 ){
			mcu_debug_user_printf("FATFS: Write retries: %d\n", retries);
		}

		if( retries == MAX_RETRIES ){
			return -1;
		}
	}

	return nbyte;
}

int fatfs_dev_read(BYTE pdrv, int loc, void * buf, int nbyte){
	int offset;
	int ret;
	char * bufp = (char*)buf;
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	if ( cfgp->file->fs == NULL ){
        return -1;
	}

	int retries;

	fatfs_dev_waitbusy(pdrv);

	//set the location to the location of the blocks
	for(offset = 0; offset < nbyte; offset += 512 ){
		retries = 0;
		do {
			cfgp->file->loc = loc;
			ret = sysfs_file_read(cfgp->file, bufp + offset, 512);
			retries++;
		} while( (retries < MAX_RETRIES) && (ret != 512) );
		loc++;

		if( retries > 1 ){
			mcu_debug_user_printf("FATFS: Write retries: %d\n", retries);
		}

		if( retries == MAX_RETRIES ){
			return -1;
		}
	}

	return nbyte;
}

int fatfs_dev_getinfo(BYTE pdrv, drive_info_t * info){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	if ( cfgp->file->fs == NULL ){
        return -1;
	}

	if( sysfs_file_ioctl(cfgp->file, I_DRIVE_GETINFO, info) < 0 ){
		return -1;
	}

	return 0;
}


int fatfs_dev_erase(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	drive_attr_t attr;
	if ( cfgp->file->fs == NULL ){
        return -1;
	}

	attr.o_flags = DRIVE_FLAG_ERASE_DEVICE;

	if( sysfs_file_ioctl(cfgp->file, I_DRIVE_SETATTR, &attr) < 0 ){
        return -1;
    }

	fatfs_dev_waitbusy(pdrv);

	return 0;
}

int fatfs_dev_waitbusy(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	if ( cfgp->file->fs == NULL ){
        return -1;
	}

	while( sysfs_file_ioctl(cfgp->file, I_DRIVE_ISBUSY, 0) > 0 ){
		usleep(1000);
	}

	return 0;
}


int fatfs_dev_eraseblocks(BYTE pdrv, int start, int end){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	drive_attr_t attr;
	//drive_erase_block_t deb;
	if ( cfgp->file->fs == NULL ){
        return -1;
	}

	attr.o_flags = DRIVE_FLAG_ERASE_BLOCKS;
	attr.start = start;
	attr.end = end;

	if( sysfs_file_ioctl(cfgp->file, I_DRIVE_SETATTR, &attr) < 0 ){
        return -1;
    }

	fatfs_dev_waitbusy(pdrv);

	return 0;
}

int fatfs_dev_close(BYTE pdrv){
	return 0;
}

