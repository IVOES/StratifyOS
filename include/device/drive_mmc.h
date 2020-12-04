/* Copyright 2011-2017 Tyler Gilbert;
 * This file is part of Stratify OS.
 *
 * Stratify OS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Stratify OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Stratify OS.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#ifndef DEVICE_MMC_H_
#define DEVICE_MMC_H_

#include "mcu/mmc.h"
#include "sos/dev/drive.h"
#include "sos/fs/devfs.h"

typedef struct {
  const char *buf;
  int *nbyte;
  int count;
  int timeout;
  uint8_t cmd[16];
  devfs_async_t op;
  mcu_event_handler_t handler;
  u32 flags;
} drive_mmc_state_t;

typedef struct {
  devfs_device_t device;
} drive_mmc_config_t;

int drive_mmc_open(const devfs_handle_t *handle);
int drive_mmc_ioctl(const devfs_handle_t *handle, int request, void *ctl);
int drive_mmc_read(const devfs_handle_t *handle, devfs_async_t *rop);
int drive_mmc_write(const devfs_handle_t *handle, devfs_async_t *wop);
int drive_mmc_close(const devfs_handle_t *handle);

int drive_mmc_dma_open(const devfs_handle_t *handle);
int drive_mmc_dma_ioctl(const devfs_handle_t *handle, int request, void *ctl);
int drive_mmc_dma_read(const devfs_handle_t *handle, devfs_async_t *rop);
int drive_mmc_dma_write(const devfs_handle_t *handle, devfs_async_t *wop);
int drive_mmc_dma_close(const devfs_handle_t *handle);

#endif /* DEVICE_MMC_H_ */
