//Copyright 2011-2016 Tyler Gilbert; All Rights Reserved


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <mcu/debug.h>

#include "ff.h"
#include "fatfs.h"
#include "fatfs_dev.h"




static int decode_result(FRESULT r){
	switch(r){
		case FR_OK: return 0;
		case FR_DISK_ERR: return EIO;
		case FR_INT_ERR: return EIO;
		case FR_NOT_READY: return EIO;
		case FR_NO_FILE: return ENOENT;
		case FR_NO_PATH: return ENOENT;
		case FR_INVALID_NAME: return EINVAL;
		case FR_DENIED: return EACCES;
		case FR_EXIST: return EEXIST;
		case FR_INVALID_OBJECT: return EINVAL;
		case FR_WRITE_PROTECTED: return EROFS;
		case FR_INVALID_DRIVE: return ENOENT;
		case FR_NOT_ENABLED: return EIO;
		case FR_NO_FILESYSTEM: return EIO;
		case FR_MKFS_ABORTED: return EIO;
		case FR_TIMEOUT: return EAGAIN;
		case FR_LOCKED: return EIO;
		case FR_NOT_ENOUGH_CORE: return ENOMEM;
		case FR_TOO_MANY_OPEN_FILES: return ENFILE;
		case FR_INVALID_PARAMETER: return EINVAL;
	}
	return ENOTSUP;
}

extern int ff_force_unlock(int volume);

void fatfs_unlock(const void * cfg){ //force unlock when a process exits
	const fatfs_config_t * fcfg = (const fatfs_config_t *)cfg;
	ff_force_unlock(fcfg->vol_id);
}

static u8 flags_to_fat(int flags){
	uint8_t f_mode = 0;
	f_mode = 0;
	if( (flags & O_ACCMODE) == O_RDONLY ){
		f_mode = FA_READ;
	} else if( (flags & O_ACCMODE) == O_WRONLY ){
		f_mode = FA_WRITE;
	} else if( (flags & O_ACCMODE) == O_RDWR ){
		f_mode = FA_READ | FA_WRITE;
	}

	if( f_mode & FA_WRITE ){
		if( (flags & O_CREAT) && (flags & O_EXCL) ){
			f_mode |= FA_CREATE_NEW; //only create if it doesn't already exist
		} else if( (flags & O_CREAT) && (flags & O_TRUNC) ){
			f_mode |= (FA_CREATE_ALWAYS); //always create (truncate if necessary)
		} else if( (flags & O_CREAT) ){
			f_mode |= FA_OPEN_ALWAYS;  //always open (create if necessary)
		}
	}

	return f_mode;
}

static int mode_to_posix(uint8_t f_mode){
	int mode;
	mode = 0;

	if( f_mode & AM_RDO ){
		//Read only
		mode = 0444;
	} else {
		//read write
		mode = 0666;
	}

	if( f_mode & AM_DIR ){
		//directory
		mode |= S_IFDIR;
	} else {
		//regular file
		mode |= S_IFREG;
	}

	//fatfs doesn't support idea of executable -- make all executable
	mode |= (0111);

	return mode;
}

static void build_ff_drive(const void * cfg, char drive[3]){
	const fatfs_config_t * fcfg = (const fatfs_config_t *)cfg;
	drive[0] = '0' + fcfg->vol_id;
	drive[1] = ':';
	drive[2] = 0;
}

static void build_ff_path(const void * cfg, char * p, const char * path){
	char c[3];
	build_ff_drive(cfg, c);
	strncpy(p, c, PATH_MAX);
	strncat(p, path, PATH_MAX);
	char * last	= p + strnlen(p, PATH_MAX) - 1;
	//doesn't like trailing /'s
	if( *last == '/' ){
		*last = 0;
	}

}

int fatfs_mount(const void * cfg){
	FRESULT result;
	char p[3];

	if( fatfs_ismounted(cfg) != 0 ){
		return 0; //already mounted
	}

	//tell the device driver which volume ID is associated with which cfg
	fatfs_dev_cfg_volume(cfg);

	build_ff_drive(cfg, p);
	//mount this volume
	result = f_mount(&FATFS_STATE(cfg)->fs, p, 1);
	if( result != FR_OK ){
		mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "failed to mount %d", result);
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return 0;
}

int fatfs_unmount(const void * cfg){
	FRESULT result;
	char p[3];

	if( fatfs_ismounted(cfg) == 0 ){
		return 0; //not mounted
	}

	//unmount this volume
	build_ff_drive(cfg, p);
	result = f_mount(&FATFS_STATE(cfg)->fs, p, 0);
	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}
	FATFS_STATE(cfg)->fs.fs_type = 0;

	return 0;

}

int fatfs_ismounted(const void * cfg){
	return (FATFS_STATE(cfg)->fs.fs_type != 0);
}


int fatfs_mkfs(const void * cfg){
	FRESULT result;
	char p[3];
	build_ff_drive(cfg, p);
	result = f_mkfs(p, 0, 0);
	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return 0;
}

int fatfs_mkdir(const void * cfg, const char * path, mode_t mode){
	FRESULT result;
	char p[PATH_MAX];

	build_ff_path(cfg, p, path);

	result = f_mkdir(p);

	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return 0;
}

int fatfs_rmdir(const void * cfg, const char * path){
	return fatfs_unlink(cfg, path);
}

int fatfs_rename(const void * cfg, const char * path_old, const char * path_new){
	char old[PATH_MAX];
	char new[PATH_MAX];
	FRESULT result;

	build_ff_path(cfg, old, path_old);
	build_ff_path(cfg, new, path_new);

	result = f_rename(old, new);

	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return 0;
}


int fatfs_opendir(const void * cfg, void ** handle, const char * path){
	FDIR * h;
	FRESULT result;
	char p[PATH_MAX];
	build_ff_path(cfg, p, path);


	h = malloc(sizeof(FDIR));
	if( h == 0 ){
		return SYSFS_SET_RETURN(ENOMEM);
	}


	result = f_opendir(h, p);

	if( result != FR_OK ){
		free(h);
		return SYSFS_SET_RETURN(decode_result(result));
	}

	*handle = h;
	return 0;
}

int fatfs_chmod(const void * cfg, const char * path, int mode){
	FRESULT result;
	uint8_t fattrib;
	char p[PATH_MAX];
	build_ff_path(cfg, p, path);
	//convert mode from POSIX to FAT
	if( mode == 0666 ){
		fattrib = FA_READ | FA_WRITE;
	} else if( mode == 0444 ){
		fattrib = FA_READ;
	} else if( mode == 0222 ){
		fattrib = FA_WRITE;
	} else {
		return SYSFS_SET_RETURN(EINVAL);
	}

	result = f_chmod(p, fattrib, FA_WRITE | FA_READ);

	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return 0;
}

int fatfs_readdir_r(const void * cfg, void * handle, int loc, struct dirent * entry){
	FRESULT result;
	FILINFO file_info;
	FDIR * dp = handle;
	char lfn[NAME_MAX];

	memset(lfn, 0, NAME_MAX);
	file_info.lfname = lfn;
	file_info.lfsize = NAME_MAX-1;

	if( loc == 0 ){
		//rewind the directory
		f_readdir(handle, 0);
	}

	result = f_readdir(handle, &file_info);

	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}

	if( file_info.fattrib & 0x08 ){  //this is the system Drive name
		result = f_readdir(handle, &file_info);
		if( result != FR_OK ){
			return SYSFS_SET_RETURN(decode_result(result));
		}
	}

	entry->d_ino = file_info.fattrib;
	if( lfn[0] != 0 ){
		strncpy(entry->d_name, lfn, NAME_MAX-1); //only gets the short file name
	} else {
		strncpy(entry->d_name, file_info.fname, 13); //only gets the short file name
	}

	if( dp->sect == 0 ){
		//no more files
		return -1; //EOF
	}

	return 0;
}
int fatfs_closedir(const void * cfg, void ** handle){
	FRESULT result;
	int ret;

	result = f_closedir(*handle);

	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	} else {
		ret = 0;
	}

	free(*handle);
	*handle = 0;

	return ret;
}

int fatfs_fstat(const void * cfg, void * handle, struct stat * stat){
	FIL * h;
	h = handle;

	memset(stat, 0, sizeof(struct stat));

	stat->st_mode = S_IFREG;
	stat->st_size = h->fsize;
	return 0;
}

int fatfs_open(const void * cfg, void ** handle, const char * path, int flags, int mode){
	FIL * h;
	FRESULT result;
	char p[PATH_MAX];
	int f_mode;
	build_ff_path(cfg, p, path);

	h = malloc(sizeof(FIL));
	if( h == 0 ){
		mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "Open ENOMEM");
		return SYSFS_SET_RETURN(ENOMEM);
	}

	f_mode = flags_to_fat(flags);

	result = f_open(h, p, f_mode);

	if( result != FR_OK ){
		free(h);
		mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "Open : Result:%d", result);
		return SYSFS_SET_RETURN(decode_result(result));
	}

	*handle = h;
	return 0;
}

int fatfs_read(const void * cfg, void * handle, int flags, int loc, void * buf, int nbyte){
	FRESULT result;
	UINT bytes;
	FIL * f = handle;

	//need to see to loc first
	if( loc != f->fptr ){
		result = f_lseek(handle, loc);
		if( result != FR_OK ){
			mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "Read Seek Result:%d", result);
			return SYSFS_SET_RETURN(decode_result(result));
		}
	}

	result = f_read(handle, buf, nbyte, &bytes);

	if( result != FR_OK ){
		mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "Read Result:%d", result);
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return bytes;
}

int fatfs_write(const void * cfg, void * handle, int flags, int loc, const void * buf, int nbyte){
	FRESULT result;
	UINT bytes;
	FIL * f = handle;


	if( loc != f->fptr ){
		mcu_debug_log_info(MCU_DEBUG_FILESYSTEM, "Loc: %ld Ptr: %ld", loc, f->fptr);
		result = f_lseek(handle, loc);
		if( result != FR_OK ){
			mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "Write Seek Result:%d", result);
			return SYSFS_SET_RETURN(decode_result(result));
		}
	}

	result = f_write(handle, buf, nbyte, &bytes);

	if( result != FR_OK ){
		mcu_debug_log_error(MCU_DEBUG_FILESYSTEM, "Write Result:%d", result);
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return bytes;
}

int fatfs_fsync(const void * cfg, void * handle){
	FRESULT result;
	FIL * f = handle;

	result = f_sync(f);

	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return 0;
}

int fatfs_close(const void * cfg, void ** handle){
	FRESULT result;
	FIL * h;
	h = *handle;

	result = f_close(h);

	if( result != FR_OK ){
		mcu_debug_log_info(MCU_DEBUG_FILESYSTEM, "FF Close: Result:%d", result);
		return SYSFS_SET_RETURN(decode_result(result));
	}
	*handle = 0;

	free(h);
	return 0;
}

int fatfs_remove(const void * cfg, const char * path){
	//this implementation uses unlink for both directories and files
	return fatfs_unlink(cfg,path);
}

int fatfs_unlink(const void * cfg, const char * path){
	FRESULT result;
	char p[PATH_MAX];
	build_ff_path(cfg, p, path);

	result = f_unlink(p);

	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}

	return 0;
}


int fatfs_stat(const void * cfg, const char * path, struct stat * stat){
	FILINFO file_info;
	FRESULT result;
	char p[PATH_MAX];
	char lfn[NAME_MAX];
	memset(lfn, 0, NAME_MAX);
	file_info.lfname = lfn;
	file_info.lfsize = NAME_MAX-1;

	build_ff_path(cfg, p, path);

	result = f_stat(p, &file_info);

	if( result != FR_OK ){
		return SYSFS_SET_RETURN(decode_result(result));
	}

	//now convert to stat
	//file_info.fdate;

	memset(stat, 0, sizeof(struct stat));
	stat->st_mode = mode_to_posix(file_info.fattrib);
	stat->st_size = file_info.fsize;
	stat->st_mtime = file_info.ftime;

	return 0;
}
