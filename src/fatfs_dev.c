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
    const fatfs_config_t * cfg = cfg_table[pdrv];
	drive_attr_t attr;
    int result;

    if( (result = sysfs_drive_open(FATFS_DRIVE(cfg))) < 0 ){
        return result;
	}

	attr.o_flags = DRIVE_FLAG_INIT;
    return sysfs_drive_ioctl(FATFS_DRIVE(cfg), I_DRIVE_SETATTR, &attr);
}

int fatfs_dev_status(BYTE pdrv){
    const fatfs_config_t * cfg = cfg_table[pdrv];

    if( FATFS_CONFIG(cfg)->drive.state->file.fs != 0 ){
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

	fatfs_dev_waitbusy(pdrv);

	for(offset = 0; offset < nbyte; offset += 512 ){
		retries = 0;
		do {
            ret = sysfs_drive_write(FATFS_DRIVE(cfgp), loc, bufp + offset, 512);

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

	int retries;

	fatfs_dev_waitbusy(pdrv);

	//set the location to the location of the blocks
	for(offset = 0; offset < nbyte; offset += 512 ){
		retries = 0;
		do {
            ret = sysfs_drive_read(FATFS_DRIVE(cfgp), loc, bufp + offset, 512);
			retries++;
		} while( (retries < MAX_RETRIES) && (ret != 512) );
		loc++;

		if( retries > 1 ){
            mcu_debug_user_printf("FATFS: Read retries: %d\n", retries);
		}

		if( retries == MAX_RETRIES ){
			return -1;
		}
	}

	return nbyte;
}

int fatfs_dev_getinfo(BYTE pdrv, drive_info_t * info){
	const fatfs_config_t * cfgp = cfg_table[pdrv];

    if( sysfs_drive_ioctl(FATFS_DRIVE(cfgp), I_DRIVE_GETINFO, info) < 0 ){
		return -1;
	}

	return 0;
}


int fatfs_dev_erase(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	drive_attr_t attr;

	attr.o_flags = DRIVE_FLAG_ERASE_DEVICE;

    if( sysfs_drive_ioctl(FATFS_DRIVE(cfgp), I_DRIVE_SETATTR, &attr) < 0 ){
        return -1;
    }

	fatfs_dev_waitbusy(pdrv);

	return 0;
}

int fatfs_dev_waitbusy(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
    u32 i = 0;

    while( (sysfs_drive_ioctl(FATFS_DRIVE(cfgp), I_DRIVE_ISBUSY, 0) > 0) && (i < 50) ){
        ;
	}

	return 0;
}


int fatfs_dev_eraseblocks(BYTE pdrv, int start, int end){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	drive_attr_t attr;
	//drive_erase_block_t deb;

	attr.o_flags = DRIVE_FLAG_ERASE_BLOCKS;
	attr.start = start;
	attr.end = end;

    if( sysfs_drive_ioctl(FATFS_DRIVE(cfgp), I_DRIVE_SETATTR, &attr) < 0 ){
        return -1;
    }

	fatfs_dev_waitbusy(pdrv);

	return 0;
}

int fatfs_dev_close(BYTE pdrv){
	return 0;
}

