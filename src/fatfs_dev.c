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
#include <sos/dev/tmr.h>
#include <sys/lock.h>
#include <time.h>
#include <mcu/core.h>
#include <mcu/debug.h>

#include "ffconf.h"
#include "fatfs.h"
#include "fatfs_dev.h"

#define MAX_RETRIES 4

extern u32 scheduler_timing_get_realtime();


DWORD get_fattime(){
	return time(0);
}

static const void * cfg_table[_VOLUMES];

static int reinitalize_drive(BYTE pdrv);

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

int reinitalize_drive(BYTE pdrv){
	const fatfs_config_t * cfg = cfg_table[pdrv];
#if 1
	drive_attr_t attr;
	attr.o_flags = DRIVE_FLAG_RESET;
	sysfs_shared_ioctl(FATFS_DRIVE(cfg), I_DRIVE_SETATTR, &attr);
#else
	mcu_action_t action;
	memset(&action, 0, sizeof(action));
	action.o_events = MCU_EVENT_FLAG_DATA_READY | MCU_EVENT_FLAG_WRITE_COMPLETE;
	sysfs_shared_ioctl(FATFS_DRIVE(cfg), I_MCU_SETACTION, &action);
	fatfs_dev_close(pdrv);
	usleep(1000);
	fatfs_dev_open(pdrv);
#endif
	fatfs_dev_waitbusy(pdrv);
	return 0;
}

int fatfs_dev_open(BYTE pdrv){
	const fatfs_config_t * cfg = cfg_table[pdrv];
	int result;
	drive_attr_t attr;

	if (cfg == 0) {
		mcu_debug_printf("FATFS: error, open with no cfg\n");
		return 1;
	}

	if( FATFS_STATE(cfg)->drive.file.handle != 0 ){
		//already initialized
		return SYSFS_RETURN_SUCCESS;
	}

	if( (result = sysfs_shared_open(FATFS_DRIVE(cfg))) < 0 ){
		return result;
	}
	attr.o_flags = DRIVE_FLAG_INIT;

	return sysfs_shared_ioctl(FATFS_DRIVE(cfg), I_DRIVE_SETATTR, &attr);
}


int fatfs_dev_status(BYTE pdrv){
	const fatfs_config_t * cfg = cfg_table[pdrv];

	if( FATFS_CONFIG(cfg)->drive.state->file.fs != 0 ){
		return 1;
	}

	mcu_debug_printf("FATFS: No Dev\n");

	return 0;
}

int fatfs_dev_write(BYTE pdrv, int loc, const void * buf, int nbyte){
	int ret;
	const char * bufp = buf;
	int retries;
	const fatfs_config_t * cfgp = cfg_table[pdrv];

	retries = 0;
	do {
		fatfs_dev_waitbusy(pdrv);
		ret = sysfs_shared_write(FATFS_DRIVE(cfgp), loc, bufp, nbyte);
		if( ret != nbyte ){
			reinitalize_drive(pdrv);
		}
		retries++;
	} while( (retries < MAX_RETRIES) && (ret != nbyte) );
	loc++;

	if( retries > 1 ){
		mcu_debug_printf("FATFS: Write retries: %d (%d, %d) 0x%X (%d)\n", retries, ret, errno, ret, loc);
	}

	if( retries == MAX_RETRIES ){
		return -1;
	}

	return nbyte;
}

int fatfs_dev_read(BYTE pdrv, int loc, void * buf, int nbyte){
	int ret;
	char * bufp = (char*)buf;
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	int retries;

	fatfs_dev_waitbusy(pdrv);
	//set the location to the location of the blocks
	retries = 0;
	do {
		ret = sysfs_shared_read(FATFS_DRIVE(cfgp), loc, bufp, nbyte);
		if( ret != nbyte ){
			reinitalize_drive(pdrv);
			fatfs_dev_waitbusy(pdrv);
		}
		retries++;
	} while( (retries < MAX_RETRIES) && (ret != nbyte) );
	loc++;

	if( retries > 1 ){
		mcu_debug_printf("FATFS: Read retries: %d\n", retries);
	}

	if( retries == MAX_RETRIES ){
		return -1;
	}


	return nbyte;
}

int fatfs_dev_getinfo(BYTE pdrv, drive_info_t * info){
	const fatfs_config_t * cfgp = cfg_table[pdrv];

	if( sysfs_shared_ioctl(FATFS_DRIVE(cfgp), I_DRIVE_GETINFO, info) < 0 ){
		mcu_debug_printf("Failed to get info\n");
		return -1;
	}

	return 0;
}


int fatfs_dev_erase(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	drive_attr_t attr;

	attr.o_flags = DRIVE_FLAG_ERASE_DEVICE;

	fatfs_dev_waitbusy(pdrv);
	if( sysfs_shared_ioctl(FATFS_DRIVE(cfgp), I_DRIVE_SETATTR, &attr) < 0 ){
		mcu_debug_printf("Failed to erase\n");
		return -1;
	}

	return 0;
}

int fatfs_dev_waitbusy(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	int result;

	while( (result = sysfs_shared_ioctl(FATFS_DRIVE(cfgp), I_DRIVE_ISBUSY, 0) > 0) ){
		if( cfgp->wait_busy_microseconds ){
			usleep(cfgp->wait_busy_microseconds);
		}
	}

	if( result < 0 ){
		mcu_debug_printf("Wait failed %d\n", result);
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

	fatfs_dev_waitbusy(pdrv);
	if( sysfs_shared_ioctl(FATFS_DRIVE(cfgp), I_DRIVE_SETATTR, &attr) < 0 ){
		mcu_debug_printf("Failed to erase block %d to %d\n", start, end);
		return -1;
	}


	return 0;
}

int fatfs_dev_close(BYTE pdrv){
	const fatfs_config_t * cfgp = cfg_table[pdrv];
	return sysfs_shared_close(FATFS_DRIVE(cfgp));
}

