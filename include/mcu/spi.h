// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

/*! \addtogroup SPI_DEV SPI
 * @{
 *
 * \ingroup DEV
 */

#ifndef _MCU_SPI_H_
#define _MCU_SPI_H_

#include "sos/dev/spi.h"
#include "sos/fs/devfs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MCU_PACK {
  u32 value;
} spi_event_data_t;

typedef struct MCU_PACK {
  u32 port;
  spi_attr_t attr; // default attributes
} spi_config_t;

#define MCU_SPI_IOCTL_REQUEST_DECLARATION(driver_name)                                   \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, getinfo);                           \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, setattr);                           \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, setaction);                         \
  DEVFS_DRIVER_DECLARTION_IOCTL_REQUEST(driver_name, swap)

#define MCU_SPI_DRIVER_DECLARATION(variant)                                              \
  DEVFS_DRIVER_DECLARTION(variant);                                                      \
  MCU_SPI_IOCTL_REQUEST_DECLARATION(variant)

MCU_SPI_DRIVER_DECLARATION(mcu_spi);
MCU_SPI_DRIVER_DECLARATION(mcu_spi_dma);
MCU_SPI_DRIVER_DECLARATION(mcu_ssp);

#define SPI_DEFINE_ATTR(                                                                 \
  attr_flags, attr_freq, attr_width, attr_miso_port, attr_miso_pin, attr_mosi_port,      \
  attr_mosi_pin, attr_sck_port, attr_sck_pin, attr_cs_port, attr_cs_pin)                 \
  .o_flags = attr_flags, .freq = attr_freq, .width = attr_width,                         \
  .pin_assignment.miso = {attr_miso_port, attr_miso_pin},                                \
  .pin_assignment.mosi = {attr_mosi_port, attr_mosi_pin},                                \
  .pin_assignment.sck = {attr_sck_port, attr_sck_pin},                                   \
  .pin_assignment.cs = {attr_cs_port, attr_cs_pin}

#define SPI_DECLARE_CONFIG(                                                              \
  name, attr_flags, attr_freq, attr_width, attr_miso_port, attr_miso_pin,                \
  attr_mosi_port, attr_mosi_pin, attr_sck_port, attr_sck_pin, attr_cs_port, attr_cs_pin) \
  const spi_config_t name##_config = {                                                   \
    .attr = {SPI_DEFINE_ATTR(                                                            \
      attr_flags, attr_freq, attr_width, attr_miso_port, attr_miso_pin, attr_mosi_port,  \
      attr_mosi_pin, attr_sck_port, attr_sck_pin, attr_cs_port, attr_cs_pin)}}

#ifdef __cplusplus
}
#endif

#endif /* _MCU_SPI_H_ */

/*! @} */
