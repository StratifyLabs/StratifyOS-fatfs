/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

//Copyright 2013-2016 Tyler Gilbert;

#include <mcu/debug.h>
#include "diskio.h"		/* FatFs lower layer API */
//#include "usbdisk.h"	/* Example: USB drive control */
//#include "atadrive.h"	/* Example: ATA drive control */
//#include "sdcard.h"		/* Example: MMC/SDC contorl */

#include "fatfs_dev.h"

char to_ascii(unsigned char c){
	if((c  >= 32)  && (c <= 126)){
		return c;
	}

	return '.';
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{

	if( fatfs_dev_open(pdrv) < 0 ){
		return STA_NOINIT;
	}



	return 0;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	if( fatfs_dev_status(pdrv) > 0 ){
		return RES_OK;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	int ret;
	ret = fatfs_dev_read(pdrv, sector, buff, count*512);
	if( ret == count*512 ){
		return RES_OK;
	}

    mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "Failed to read disk");
    return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	int ret;
	ret = fatfs_dev_write(pdrv, sector, buff, count*512);
	if( ret == count*512 ){
		return RES_OK;
	}

    mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "Failed to write disk");
    return RES_ERROR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	drive_info_t info;

	DWORD * dp, st, end;

	dp = buff;

	switch(cmd){
	case CTRL_SYNC:
		//wait while the disk is busy
		fatfs_dev_waitbusy(pdrv);

		return RES_OK;

	case GET_SECTOR_COUNT:
		//This is the
		if( fatfs_dev_getinfo(pdrv, &info) < 0 ){
			return RES_ERROR;
		}

		dp[0] = info.num_write_blocks;

		return RES_OK;
	case GET_BLOCK_SIZE: //eraseable block size

		if( fatfs_dev_getinfo(pdrv, &info) < 0 ){
			return RES_ERROR;
		}

		dp[0] = info.erase_block_size / info.write_block_size;

		return RES_OK;
	case CTRL_ERASE_SECTOR:
		dp = buff;
		st = dp[0];
		end = dp[1];
		//erase sectors st to end
		fatfs_dev_eraseblocks(pdrv, st, end);
		return RES_OK;
	}

	return RES_OK;
}
#endif
