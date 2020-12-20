// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md


#include "crt_common.h"

extern u32 _signature;
extern u32 _app_ram_size;

void crt(char *path_arg);

void *__dso_handle;

extern struct _reent global_impure_data __attribute__((section(".global_reent")));
struct _reent global_impure_data = _REENT_INIT(global_impure_data);

const appfs_file_t startup_data __attribute__((section(".startup"))) = {
  .hdr.name = "",
  .hdr.mode = 0555,
  .hdr.version = 0x0000,
  .hdr.id = "",
  .exec.startup = (u32)crt,
  .exec.code_start = (u32)&_text,
  .exec.code_size = (u32)&_text_size,
  .exec.ram_start = (u32)&_data,
  .exec.ram_size = (int)&_app_ram_size,
  .exec.data_size = (int)&_data_size,
  .exec.o_flags = APPFS_FLAG_IS_FLASH,
  .exec.signature = (u32)&_signature};

void crt(char *path_arg) {
  int ret;

  // Copy the heap to RAM -- since the heap copy is in no man's land this needs to be
  // privileged
  crt_load_data(
    &global_impure_data, startup_data.exec.code_size, startup_data.exec.data_size);

  if (crt_common(path_arg, &ret, startup_data.hdr.name) != 0) {
    ret = -1;
  }

  exit(ret);
}
