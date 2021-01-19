// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#include "cortexm/mpu.h"
#include "cortexm_local.h"

//  __I  u32 TYPE;                         /*!< Offset: 0x00  MPU Type Register */
//  __IO u32 CTRL;                         /*!< Offset: 0x04  MPU Control Register */
//  __IO u32 RNR;                          /*!< Offset: 0x08  MPU Region RNR Register */
//  __IO u32 RBAR;                         /*!< Offset: 0x0C  MPU Region Base Address
//  Register               */
//  __IO u32 RASR;                         /*!< Offset: 0x10  MPU Region Attribute and
//  Size Register         */
//  __IO u32 RBAR_A1;                      /*!< Offset: 0x14  MPU Alias 1 Region Base
//  Address Register       */
//  __IO u32 RASR_A1;                      /*!< Offset: 0x18  MPU Alias 1 Region Attribute
//  and Size Register */
//  __IO u32 RBAR_A2;                      /*!< Offset: 0x1C  MPU Alias 2 Region Base
//  Address Register       */
//  __IO u32 RASR_A2;                      /*!< Offset: 0x20  MPU Alias 2 Region Attribute
//  and Size Register */
//  __IO u32 RBAR_A3;                      /*!< Offset: 0x24  MPU Alias 3 Region Base
//  Address Register       */
//  __IO u32 RASR_A3;                      /*!< Offset: 0x28  MPU Alias 3 Region Attribute
//  and Size Register */

int mpu_enable() {
  MPU->CTRL =
    (1 << 0) | (1 << 2); // enable the MPU -- give priv software access to background map
  return 0;
}

int mpu_disable() {
  MPU->CTRL = 0; // disable the MPU
  return 0;
}

mpu_size_t mpu_calc_size(u32 size) {
  u32 shift;
  shift = 5;
  while ((u32)(1 << shift) < (u32)size) {
    shift++;
    if (shift == (MPU_SIZE_4GB + 1)) {
      // size is too big
      return 0;
    }
  }
  return (mpu_size_t)(shift - 1);
}

int mpu_disable_region(int region) {
  u32 rasr;
  u8 valid_regions;
  // check if the region is valid
  valid_regions = ((MPU->TYPE >> 8) & 0xFF);
  if (region >= valid_regions) {
    return -1;
  }

  sos_debug_printf("disable %d\n", region);
  MPU->RNR = (region & 0xFF); // Set the region register
  rasr = MPU->RASR;
  rasr &= ~(0x01);
  MPU->RASR = rasr;
  return 0;
}

u32 mpu_calc_region(
  int region,
  const void *addr,
  u32 size,
  mpu_access_t access,
  mpu_memory_t type,
  int executable,
  u32 *rbar,
  u32 *rasr) {

  u32 rbar_value = (1 << 4) | region;
  u32 rasr_value = 0;

  *rbar = rbar_value;
  *rasr = rasr_value;

  const u32 target_address = (u32)addr;
  u32 aligned_address = target_address;
  u32 aligned_size;
  u32 subregion_size;
  u32 subregion_count;
  u32 subregion_offset;
  u32 max_subregion_size;
  max_subregion_size = 0;
  subregion_size = 0;

  if (size < 32) {
    return 0; // too small, can't protect less than 32 bytes
  }

  do {
    subregion_count = 0;
    subregion_offset = 0;
    // zoom out the aligned address until something fits
    aligned_size = 1 << (__CLZ(__RBIT((u32)aligned_address)));
    if (size <= aligned_size || (aligned_size == 0)) { // aligned size = 0 means 4GB of
                                                       // size
      if (aligned_size == 0) {
        max_subregion_size = 1 << 29;
      } else {
        max_subregion_size = aligned_size >> 3;
      }

      if (max_subregion_size > (u32)size) {
        max_subregion_size = size;
      }

      // check subregion sizes from smallest to largest
      for (subregion_size = 32; subregion_size <= max_subregion_size;
           subregion_size <<= 1) {

        if (target_address % subregion_size == 0) { // check that the address is aligned
                                                    // to a subregion boundary
          subregion_count = (size + subregion_size - 1)
                            / subregion_size; // round size up to capture all memory
          subregion_offset = (target_address - aligned_address) / subregion_size;
          if (subregion_count + subregion_offset > 8) {
            subregion_count = 0;
            subregion_offset = 0;
          } else {
            break;
          }
        }
      }
    }

    if (subregion_count == 0) {
      aligned_address -= aligned_size;
    }
  } while ((subregion_count == 0) && (max_subregion_size != size));

  if (subregion_count == 0) {
    return 0;
  }

  // Check the size
  u32 mpu_size = 31 - __CLZ(subregion_size * 8) - 1;
  if (mpu_size < 4) {
    return 0;
  }

  u8 subregion_disable_mask = 0xFF;
  for (u8 i = 0; i < subregion_count; i++) {
    subregion_disable_mask &= ~(1 << (subregion_offset + i));
  }

  rasr_value =
    (mpu_size << 1) | (1 << 0) | (access << 24) | (subregion_disable_mask << 8);

  if (!executable) {
    rasr_value |= (1 << 28);
  }

  /*
   * Bits 19, 20, 21 = TEX
   * Bit 18 = shareable (can't be cached?) (S)
   * Bit 17 = cacheable (C)
   * Bit 16 = write-back (B)
   *
   *
   */
  const u32 default_cache_policy =
    (1 << 21) | (1 << 20) | (1 << 19) | (0 << 18) | (1 << 17) | (0 << 16);

  switch (type) {
  case MPU_MEMORY_EXTERNAL_SRAM:
    rasr_value |= default_cache_policy; // Outer and Inner Write-Back, no Write-Allocate
    break;
  case MPU_MEMORY_SRAM:
    rasr_value |= default_cache_policy; // Outer and Inner Write-Back, no Write-Allocate
    break;
  case MPU_MEMORY_FLASH:
    rasr_value |= default_cache_policy; // Outer and Inner Write-Back, no Write-Allocate
    break;
  case MPU_MEMORY_LCD:
  case MPU_MEMORY_PERIPHERALS:
    rasr_value |= ((1 << 16) | (1 << 18)); // shareable, not cacheable
    break;
  }

  rbar_value = ((u32)aligned_address) | (1 << 4) | region;
  *rasr = rasr_value;
  *rbar = rbar_value;

  return subregion_size * subregion_count;
}

int mpu_enable_region(
  int region,
  const void *addr,
  u32 size,
  mpu_access_t access,
  mpu_memory_t type,
  int executable) {
  int err;
  u32 rasr;
  u32 rbar;
  u8 valid_regions;

  sos_debug_printf("enable %d\n", region);

  // check if the region is valid
  valid_regions = ((MPU->TYPE >> 8) & 0xFF);
  if (region >= valid_regions) {
    return -1;
  }

  err = mpu_calc_region(region, addr, size, access, type, executable, &rbar, &rasr);
  if (err == 0) {
    return -1;
  }

  __DSB();
  MPU->RBAR = rbar;
  MPU->RASR = rasr;
  MPU->RNR = 15;

  return 0;
}
