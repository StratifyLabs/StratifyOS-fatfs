// Copyright 2015-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#ifndef FATFS_FATFS_H_
#define FATFS_FATFS_H_

#include <sdk/types.h>
#include <sos/fs/sysfs.h>
#include <sys/lock.h>

#include "fatfs/ff.h"

typedef struct {
  sysfs_shared_state_t drive;
  FATFS fs;
} fatfs_state_t;

typedef struct {
  u32 block_offset;
  u32 block_count;
} fatfs_config_partition_t;

typedef struct {
  sysfs_shared_config_t drive;
  fatfs_config_partition_t partition;
  int (*is_drive_present)();
  u16 wait_busy_microseconds;
  u16 wait_busy_timeout_count;
  u8 vol_id;
} fatfs_config_t;

#define FATFS_DECLARE_CONFIG_STATE(                                                                       \
  config_name,                                                                                            \
  devfs_value,                                                                                            \
  device_name,                                                                                            \
  vol_id_value,                                                                                           \
  wait_busy_microseconds_value,                                                                           \
  wait_busy_timeout_count_value)                                                                          \
  fatfs_state_t config_name##_state;                                                                      \
  const fatfs_config_t config_name##_config = {                                                           \
    .drive                                                                                                \
    = {.devfs = devfs_value, .name = device_name, .state = (sysfs_shared_state_t *)&config_name##_state}, \
    .vol_id = vol_id_value,                                                                               \
    .wait_busy_microseconds = wait_busy_microseconds_value,                                               \
    .wait_busy_timeout_count = wait_busy_timeout_count_value,                                             \
    .partition.block_offset = 0,                                                                          \
    .partition.block_count = 0}

#define FATFS_DECLARE_CONFIG_STATE_PARTITION(                                                             \
  config_name,                                                                                            \
  devfs_value,                                                                                            \
  device_name,                                                                                            \
  vol_id_value,                                                                                           \
  wait_busy_microseconds_value,                                                                           \
  wait_busy_timeout_count_value,                                                                          \
  partition_block_offset_value,                                                                           \
  partition_block_count_value)                                                                            \
  fatfs_state_t config_name##_state;                                                                      \
  const fatfs_config_t config_name##_config = {                                                           \
    .drive                                                                                                \
    = {.devfs = devfs_value, .name = device_name, .state = (sysfs_shared_state_t *)&config_name##_state}, \
    .vol_id = vol_id_value,                                                                               \
    .wait_busy_microseconds = wait_busy_microseconds_value,                                               \
    .wait_busy_timeout_count = wait_busy_timeout_count_value,                                             \
    .partition.block_offset = partition_block_offset_value,                                               \
    .partition.block_count = partition_block_count_value}

int fatfs_mount(const void *cfg);     // initialize the filesystem
int fatfs_unmount(const void *cfg);   // initialize the filesystem
int fatfs_ismounted(const void *cfg); // initialize the filesystem
int fatfs_mkfs(const void *cfg);

int fatfs_opendir(const void *cfg, void **handle, const char *path);
int fatfs_readdir_r(
  const void *cfg,
  void *handle,
  int loc,
  struct dirent *entry);
int fatfs_rewinddir(const void *cfg, void *handle);
int fatfs_closedir(const void *cfg, void **handle);

int fatfs_mkdir(const void *cfg, const char *path, mode_t mode);
int fatfs_rmdir(const void *cfg, const char *path);
int fatfs_rename(const void *cfg, const char *path_old, const char *path_new);
int fatfs_chmod(const void *cfg, const char *path, int mode);

int fatfs_fstat(const void *cfg, void *handle, struct stat *stat);
int fatfs_open(
  const void *cfg,
  void **handle,
  const char *path,
  int flags,
  int mode);
int fatfs_read(
  const void *cfg,
  void *handle,
  int flags,
  int loc,
  void *buf,
  int nbyte);
int fatfs_write(
  const void *cfg,
  void *handle,
  int flags,
  int loc,
  const void *buf,
  int nbyte);
int fatfs_fsync(const void *cfg, void *handle);
int fatfs_close(const void *cfg, void **handle);
int fatfs_remove(const void *cfg, const char *path);
int fatfs_unlink(const void *cfg, const char *path);

void fatfs_unlock(const void *cfg);

int fatfs_stat(const void *cfg, const char *path, struct stat *stat);

#define FATFS_MOUNT(mount_loc_name, cfgp, permissions_value, owner_value)      \
  {                                                                            \
    .mount_path = mount_loc_name, .permissions = permissions_value,            \
    .owner = owner_value, .mount = fatfs_mount, .unmount = fatfs_unmount,      \
    .ismounted = fatfs_ismounted, .startup = SYSFS_NOTSUP, .mkfs = fatfs_mkfs, \
    .open = fatfs_open, .aio = SYSFS_NOTSUP, .fsync = fatfs_fsync,             \
    .read = fatfs_read, .write = fatfs_write, .close = fatfs_close,            \
    .ioctl = SYSFS_NOTSUP, .rename = fatfs_rename, .unlink = fatfs_unlink,     \
    .mkdir = fatfs_mkdir, .rmdir = fatfs_rmdir, .remove = fatfs_remove,        \
    .opendir = fatfs_opendir, .closedir = fatfs_closedir,                      \
    .readdir_r = fatfs_readdir_r, .link = SYSFS_NOTSUP,                        \
    .symlink = SYSFS_NOTSUP, .stat = fatfs_stat, .lstat = SYSFS_NOTSUP,        \
    .fstat = fatfs_fstat, .chmod = fatfs_chmod, .chown = SYSFS_NOTSUP,         \
    .unlock = fatfs_unlock, .config = cfgp,                                    \
  }

#endif /* FATFS_FATFS_H_ */
