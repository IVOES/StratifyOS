// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md



#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "sos/debug.h"
#include "cortexm/cortexm.h"
#include "cortexm/task.h"
#include "device/i2s_ffifo.h"

static int i2s_event_write_complete(void * context, const mcu_event_t * event);
static int i2s_event_data_ready(void * context, const mcu_event_t * event);

int i2s_event_write_complete(void * context, const mcu_event_t * event){
    const devfs_handle_t * handle = context;
    const i2s_ffifo_config_t * config = handle->config;
    i2s_ffifo_state_t * state = handle->state;
    int nbyte;
    ffifo_state_t * ffifo_state = &state->tx.ffifo;

    if (state->tx.i2s_async.result < 0) {
      state->tx.error = state->tx.i2s_async.result;
      return 0;
    }

    nbyte = state->tx.i2s_async.result;

    if( ffifo_state->atomic_position.access.head == ffifo_state->atomic_position.access.tail ){
        //no data to read -- send a zero frame
        memset(state->tx.i2s_async.buf, 0, nbyte);
    } else {
        state->tx.access_count++;

        //increment the tail for the frame that was written
        if( ffifo_state->atomic_position.access.tail == config->tx.frame_count ){
            ffifo_state->atomic_position.access.tail = ffifo_state->atomic_position.access.head;
        }
        ffifo_inc_tail(ffifo_state, config->tx.frame_count);

        if( ffifo_state->atomic_position.access.tail == 0 ){
            state->tx.i2s_async.buf = config->rx.buffer;
        } else {
            state->tx.i2s_async.buf += nbyte;
        }
    }



    state->tx.error = mcu_i2s_write(handle, &(state->tx.i2s_async));

    //because mcu_i2s_write() reassigns the callback value -- returning 0 will null the callback
    return 1;
}

int i2s_event_data_ready(void * context, const mcu_event_t * event){
    const devfs_handle_t * handle = context;
    const i2s_ffifo_config_t * config = handle->config;
    i2s_ffifo_state_t * state = handle->state;
    ffifo_state_t * ffifo_state = &state->rx.ffifo;

    if (state->rx.i2s_async.result < 0) {
      state->rx.error = state->rx.i2s_async.result;
      sos_debug_log_error(
        SOS_DEBUG_DEVICE, "%s:%d (%d,%d)", __FUNCTION__, __LINE__,
        SYSFS_GET_RETURN(state->rx.i2s_async.result));
      return 0;
    }

    //increment the head for the frame received
    if( ffifo_state->atomic_position.access.tail == config->rx.frame_count ){
        if( ffifo_state->o_flags & FIFO_FLAG_IS_READ_BUSY ){
            ffifo_state->o_flags |= FIFO_FLAG_IS_WRITE_WHILE_READ_BUSY;
        }
        ffifo_state->o_flags |= FIFO_FLAG_IS_OVERFLOW;
    }
    ffifo_inc_head(&state->rx.ffifo, config->rx.frame_count);

    state->rx.access_count++;

    if( ffifo_state->atomic_position.access.head == 0 ){
        state->rx.i2s_async.buf = config->rx.buffer;
    } else {
        state->rx.i2s_async.buf += config->rx.frame_size;
    }

    if (state->rx.i2s_async.result != config->rx.frame_size) {
      sos_debug_log_warning(
        SOS_DEBUG_DEVICE, "%s:%d != %d\n", __FUNCTION__, state->rx.i2s_async.result,
        config->rx.frame_size);
    }

    state->rx.error = mcu_i2s_read(handle, &(state->rx.i2s_async));
    if( state->rx.error < 0){
        sos_debug_log_error(SOS_DEBUG_DEVICE, "%s:%d", __FUNCTION__, __LINE__);
        return 0;
    }
    return 1;
}

int i2s_ffifo_open(const devfs_handle_t * handle){
    const i2s_ffifo_config_t * config = handle->config;
    i2s_ffifo_state_t * state = handle->state;
    int ret;

    ret = mcu_i2s_open(handle);
    if( ret <  0 ){ return ret; }

    ret = ffifo_open_local(&config->tx, &state->tx.ffifo);
    if( ret < 0 ){ return ret;}

    return ffifo_open_local(&config->rx, &state->rx.ffifo);
}

int i2s_ffifo_ioctl(const devfs_handle_t * handle, int request, void * ctl){
    const i2s_ffifo_config_t * config = handle->config;
    i2s_ffifo_state_t * state = handle->state;
    i2s_ffifo_info_t * info = ctl;
    mcu_action_t * action = ctl;
    int ret;

    switch(request){

    case I_I2S_FFIFO_GETVERSION:
        return I2S_FFIFO_VERSION;

    case I_I2S_SETATTR:
        ret = mcu_i2s_ioctl(handle, request, ctl);
        if( ret < 0 ){
            return ret;
        }
        /* no break */
    case I_FFIFO_INIT:
        //the driver reads the I2S TX FIFO to write to the I2S
        if( config->tx.buffer ){
            state->tx.i2s_async.loc = 0;
            state->tx.i2s_async.tid = task_get_current();
            state->tx.i2s_async.flags = O_RDWR;
            state->tx.i2s_async.buf = config->tx.buffer;
            state->tx.i2s_async.nbyte = config->tx.frame_size;
            state->tx.i2s_async.handler.context = (void*)handle;
            state->tx.i2s_async.handler.callback = i2s_event_write_complete;

            //write block is true but the function doesn't block -- returns EGAIN if it is written while full
            ffifo_set_writeblock(&(state->tx.ffifo), 1);

            state->tx.access_count = 0;
            state->tx.error = 0;

            state->tx.ffifo.transfer_handler.read = 0;
            state->tx.ffifo.transfer_handler.write = 0;
            ffifo_flush(&(state->tx.ffifo));

            //ffifo is empty so the head must be incremented on start
            ffifo_inc_head(&state->tx.ffifo, config->tx.frame_count);
            memset(state->tx.i2s_async.buf, 0, state->tx.i2s_async.nbyte);

            //start writing data to the I2S -- zeros are written if there is no data in the fifo
            if( mcu_i2s_write(handle, &(state->tx.i2s_async)) < 0 ){
                return SYSFS_SET_RETURN(EIO);
            }
        }


        if( config->rx.buffer ){

            //the driver writes the I2S RX FIFO that is read from the I2S
            state->rx.i2s_async.loc = 0;
            state->rx.i2s_async.tid = task_get_current();
            state->rx.i2s_async.flags = O_RDWR;
            state->rx.i2s_async.buf = config->rx.buffer;
            state->rx.i2s_async.nbyte = config->rx.frame_size;
            state->rx.i2s_async.handler.context = (void*)handle;
            state->rx.i2s_async.handler.callback = i2s_event_data_ready;

            state->rx.error = 0;
            state->rx.access_count = 0;

            state->rx.ffifo.transfer_handler.read = 0;
            state->rx.ffifo.transfer_handler.read = 0;
            ffifo_flush(&(state->rx.ffifo));

            //on the first call the FIFO is full of zeros and returns immediately
            if( mcu_i2s_read(handle, &(state->rx.i2s_async)) < 0 ){
                return SYSFS_SET_RETURN(EIO);
            }
        }

        return 0;

    case I_FFIFO_FLUSH:
        ffifo_flush(&(state->tx.ffifo));
        ffifo_flush(&(state->rx.ffifo));

        state->rx.i2s_async.buf = config->rx.buffer;
        state->rx.i2s_async.nbyte = config->rx.frame_size;

        state->tx.i2s_async.buf = config->tx.buffer;
        state->tx.i2s_async.nbyte = config->tx.frame_size;
        return 0;

    case I_FFIFO_SETATTR:
        return SYSFS_SET_RETURN(ENOTSUP);

    case I_FFIFO_GETINFO:
        return SYSFS_SET_RETURN(EINVAL);

    case I_I2S_FFIFO_GETINFO:
        ffifo_getinfo(&(info->tx.ffifo), &(config->tx), &(state->tx.ffifo));
        ffifo_getinfo(&(info->rx.ffifo), &(config->rx), &(state->rx.ffifo));
        info->tx.access_count = state->tx.access_count;
        info->rx.access_count = state->rx.access_count;
        info->tx.error = state->tx.error;
        info->rx.error = state->rx.error;
        return 0;

    case I_I2S_FFIFO_SETACTION:
    case I_I2S_SETACTION:
    case I_MCU_SETACTION:

        //this needs to handle cancelling wop/rop of tx and rx fifos
        if( (action->o_events & MCU_EVENT_FLAG_DATA_READY) && (action->handler.callback == 0) ){
            //cancel the ffifo rx
            ffifo_cancel_async_read(&(state->rx.ffifo));
        }

        //this needs to handle cancelling wop/rop of tx and rx fifos
        if( (action->o_events & MCU_EVENT_FLAG_WRITE_COMPLETE) && (action->handler.callback == 0) ){
            //cancel the ffifo rx
            ffifo_cancel_async_write(&(state->tx.ffifo));
        }
        /* no break */
    }

    return mcu_i2s_ioctl(handle, request, ctl);
}

int i2s_ffifo_read(const devfs_handle_t * handle, devfs_async_t * async){
    const i2s_ffifo_config_t * config = handle->config;
    i2s_ffifo_state_t * state = handle->state;
    int ret;

    if (async->nbyte % config->rx.frame_size != 0) {
      return SYSFS_SET_RETURN(EINVAL);
    }

    ret = ffifo_read_buffer(&(config->rx), &(state->rx.ffifo), async->buf, async->nbyte);
    if( ret == 0 ){
        ret = SYSFS_SET_RETURN(EAGAIN);
    }
    return ret;
}

int i2s_ffifo_write(const devfs_handle_t * handle, devfs_async_t * async){
    int ret;
    const i2s_ffifo_config_t * config = handle->config;
    i2s_ffifo_state_t * state = handle->state;

    if (async->nbyte % config->rx.frame_size != 0) {
      return SYSFS_SET_RETURN(EINVAL);
    }

    //I2S interrupt can't fire while writing the local buffer?
    ret = ffifo_write_buffer(&(config->tx), &(state->tx.ffifo), async->buf, async->nbyte);
    if( ret == 0 ){
        //no room
        ret = SYSFS_SET_RETURN(EAGAIN);
    }
    return ret;
}

int i2s_ffifo_close(const devfs_handle_t * handle){
    const i2s_ffifo_config_t * config = handle->config;
    i2s_ffifo_state_t * state = handle->state;
    ffifo_close_local(&(config->tx), &(state->tx.ffifo));
    ffifo_close_local(&(config->rx), &(state->rx.ffifo));
    return mcu_i2s_close(handle);
}


