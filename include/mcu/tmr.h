/* Copyright 2011-2018 Tyler Gilbert;
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

/*! \addtogroup TMR_DEV TMR
 * @{
 *
 * \ingroup DEV
 */

#ifndef _MCU_TMR_H_
#define _MCU_TMR_H_

#include "sos/dev/tmr.h"
#include "sos/fs/devfs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MCU_PACK {
  u32 value;
} tmr_event_data_t;

typedef struct MCU_PACK {
  mcu_channel_t channel;
} tmr_event_t;

typedef struct MCU_PACK {
  tmr_attr_t attr;
} tmr_config_t;

#define MCU_TMR_IOCTL_REQUEST_DECLARATION(driver_name)                                   \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, getinfo);                           \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, setattr);                           \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, setaction);                         \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, setchannel);                        \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, getchannel);                        \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, get);                               \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, set);                               \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, enable);                            \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, disable)

#define MCU_TMR_DRIVER_DECLARATION(variant)                                              \
  DEVFS_DRIVER_DECLARTION(variant);                                                      \
  MCU_TMR_IOCTL_REQUEST_DECLARATION(variant)

MCU_TMR_DRIVER_DECLARATION(mcu_state_tmr);
MCU_TMR_DRIVER_DECLARATION(mcu_tmr_pwm);
MCU_TMR_DRIVER_DECLARATION(mcu_tmr_io);

int mcu_tmr_open(const devfs_handle_t *handle) MCU_ROOT_CODE;
int mcu_tmr_read(const devfs_handle_t *handle, devfs_async_t *rop) MCU_ROOT_CODE;
int mcu_tmr_write(const devfs_handle_t *handle, devfs_async_t *wop) MCU_ROOT_CODE;
int mcu_tmr_ioctl(const devfs_handle_t *handle, int request, void *ctl) MCU_ROOT_CODE;
int mcu_tmr_close(const devfs_handle_t *handle) MCU_ROOT_CODE;

int mcu_tmr_getinfo(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;
int mcu_tmr_setattr(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;
int mcu_tmr_setaction(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;

int mcu_tmr_setchannel(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;
int mcu_tmr_getchannel(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;
int mcu_tmr_set(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;
int mcu_tmr_get(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;
int mcu_tmr_enable(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;
int mcu_tmr_disable(const devfs_handle_t *handle, void *ctl) MCU_ROOT_CODE;

#ifdef __cplusplus
}
#endif

#endif // _MCU_TMR_H_

/*! @} */
