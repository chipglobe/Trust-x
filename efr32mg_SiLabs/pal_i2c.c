/**
* MIT License
*
* Copyright (c) 2019 Arrow Electronics
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE
*
*
* \file
*
* \brief This file implements the platform abstraction layer(pal) APIs for I2C.
*
* \ingroup  grPAL
*
* @{
*/

/**********************************************************************************************************************
 * HEADER FILES
 *********************************************************************************************************************/
/* Optiga based includes */
#include <trustx/optiga/include/optiga/pal/pal_i2c.h>

#include "sl_i2cspm_instances.h"

/**********************************************************************************************************************
 * MACROS
 *********************************************************************************************************************/
#define SEM_INIT_VALUE      1
#define SEM_MAX_VALUE       1
#define SEM_TAKE_SUCCESS    0

/*********************************************************************************************************************
 * LOCAL DATA
 *********************************************************************************************************************//* Varibale to indicate the re-entrant count of the i2c bus acquire function*/
static volatile uint32_t g_entry_count = 0;

/* context for i2c devices */
typedef struct {
    sl_i2cspm_t *sl_i2cspm_sensor;
    uint32_t p_bitrate;
} i2c_ctx_t;

/**********************************************************************************************************************
 * LOCAL ROUTINES
 *********************************************************************************************************************/
// I2C acquire bus function
//lint --e{715} suppress the unused p_i2c_context variable lint error , since this is kept for future enhancements
static pal_status_t pal_i2c_acquire(const void* p_i2c_context)
{
  if((p_i2c_context == NULL)){
    return PAL_STATUS_FAILURE;
  }
  if(g_entry_count == 0)
  {
      g_entry_count++;
      if(g_entry_count == 1)
      {
          return PAL_STATUS_SUCCESS;
      }
  }
  return PAL_STATUS_FAILURE;
}

// I2C release bus function
//lint --e{715} suppress the unused p_i2c_context variable lint, since this is kept for future enhancements
static void pal_i2c_release(const void* p_i2c_context)
{
  if((p_i2c_context != NULL)){
    g_entry_count = 0;
  }
}

/**********************************************************************************************************************
 * API IMPLEMENTATION
 *********************************************************************************************************************/
/**
 * Initializes the i2c master with the given context.
 * <br>
 *
 *<b>API Details:</b>
 * - The platform specific initialization of I2C master has to be implemented as part of this API, if required.<br>
 * - If the target platform does not demand explicit initialization of i2c master
 *   (Example: If the platform driver takes care of init after the reset), it would not be required to implement.<br>
 * - The implementation must take care the following scenarios depending upon the target platform selected.
 *   - The implementation must handle the acquiring and releasing of the I2C bus before initializing the I2C master to
 *     avoid interrupting the ongoing slave I2C transactions using the same I2C master.
 *   - If the I2C bus is in busy state, the API must not initialize and return #PAL_STATUS_I2C_BUSY status.
 *   - Repeated initialization must be taken care with respect to the platform requirements. (Example: Multiple users/applications
 *     sharing the same I2C master resource)
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t p_i2c_context must not be NULL.<br>
 *
 * \param[in] p_i2c_context   Pal i2c context to be initialized
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the I2C master init it successfull
 * \retval  #PAL_STATUS_FAILURE  Returns when the I2C init fails.
 */
pal_status_t pal_i2c_init(const pal_i2c_t* p_i2c_context)
{
    i2c_ctx_t *current_ctx = p_i2c_context->p_i2c_hw_config;
    if(current_ctx->sl_i2cspm_sensor == NULL){
        return PAL_STATUS_FAILURE;
    }

    return PAL_STATUS_SUCCESS;
}

/**
 * De-initializes the I2C master with the specified context.
 * <br>
 *
 *<b>API Details:</b>
 * - The platform specific de-initialization of I2C master has to be implemented as part of this API, if required.<br>
 * - If the target platform does not demand explicit de-initialization of i2c master
 *   (Example: If the platform driver takes care of init after the reset), it would not be required to implement.<br>
 * - The implementation must take care the following scenarios depending upon the target platform selected.
 *   - The implementation must handle the acquiring and releasing of the I2C bus before de-initializing the I2C master to
 *     avoid interrupting the ongoing slave I2C transactions using the same I2C master.
 *   - If the I2C bus is in busy state, the API must not de-initialize and return #PAL_STATUS_I2C_BUSY status.
 *   - This API must ensure that multiple users/applications sharing the same I2C master resource is not impacted.
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t p_i2c_context must not be NULL.<br>
 *
 * \param[in] p_i2c_context   I2C context to be de-initialized
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the I2C master de-init it successfull
 * \retval  #PAL_STATUS_FAILURE  Returns when the I2C de-init fails.
 */
pal_status_t pal_i2c_deinit(const pal_i2c_t* p_i2c_context)
{
  pal_status_t status = PAL_STATUS_SUCCESS;
  if((p_i2c_context == NULL)){
      status = PAL_STATUS_FAILURE;
  }
  return status;
}

/**
 * Writes the data to I2C slave.
 * <br>
 * <br>
 * \image html pal_i2c_write.png "pal_i2c_write()" width=20cm
 *
 *
 *<b>API Details:</b>
 * - The API attempts to write if the I2C bus is free, else it returns busy status #PAL_STATUS_I2C_BUSY<br>
 * - The bus is released only after the completion of transmission or after completion of error handling.<br>
 * - The API invokes the upper layer handler with the respective event status as explained below.
 *   - #PAL_I2C_EVENT_BUSY when I2C bus in busy state
 *   - #PAL_I2C_EVENT_ERROR when API fails
 *   - #PAL_I2C_EVENT_SUCCESS when operation is successfully completed asynchronously
 *<br>
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t p_i2c_context must not be NULL.<br>
 * - The upper_layer_event_handler must be initialized in the p_i2c_context before invoking the API.<br>
 *
 *<b>Notes:</b><br>
 *  - Otherwise the below implementation has to be updated to handle different bitrates based on the input context.<br>
 *  - The caller of this API must take care of the guard time based on the slave's requirement.<br>
 *
 * \param[in] p_i2c_context  Pointer to the pal I2C context #pal_i2c_t
 * \param[in] p_data         Pointer to the data to be written
 * \param[in] length         Length of the data to be written
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the I2C write is invoked successfully
 * \retval  #PAL_STATUS_FAILURE  Returns when the I2C write fails.
 * \retval  #PAL_STATUS_I2C_BUSY Returns when the I2C bus is busy.
 */
pal_status_t pal_i2c_write(pal_i2c_t *p_i2c_context, uint8_t *p_data,
                           uint16_t length)
{
    pal_status_t status;
    I2C_TransferSeq_TypeDef seq;
    app_event_handler_t upper_layer_handler =
            (app_event_handler_t)p_i2c_context->upper_layer_event_handler;


    if ((PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context)) && (p_i2c_context != NULL)) {
        int i2c_result;
        seq.addr = (p_i2c_context->slave_address) << 1;
        /*if(length > 1){
            seq.flags = I2C_FLAG_WRITE_WRITE;
            seq.buf[0].len  = 1;
            seq.buf[0].data = p_data;
            seq.buf[1].len  = (uint8_t)(length-1);
            seq.buf[1].data = p_data++;
        } else if(length == 1){
            seq.flags = I2C_FLAG_WRITE;
            seq.buf[0].len  = 1;
            seq.buf[0].data = p_data;
            seq.buf[1].len  = 0;
        }*/
        seq.flags = I2C_FLAG_WRITE;
        seq.buf[0].len  = length;
        seq.buf[0].data = p_data;
        seq.buf[1].len  = 0;

        i2c_result = I2CSPM_Transfer(((i2c_ctx_t *)(p_i2c_context->p_i2c_hw_config))->sl_i2cspm_sensor, &seq);

        if (i2c_result == 0) {
            upper_layer_handler(p_i2c_context->upper_layer_ctx,
                                PAL_I2C_EVENT_SUCCESS);
            status = PAL_STATUS_SUCCESS;
        } else {
            upper_layer_handler(p_i2c_context->upper_layer_ctx,
                                PAL_I2C_EVENT_ERROR);
            status = PAL_STATUS_FAILURE;
        }

        pal_i2c_release((void *)p_i2c_context);
    } else {
        status = PAL_STATUS_I2C_BUSY;
        upper_layer_handler(p_i2c_context->upper_layer_ctx,
                            PAL_I2C_EVENT_BUSY);
    }

    return status;
}

/**
 * Reads the data from I2C slave.
 * <br>
 * <br>
 * \image html pal_i2c_read.png "pal_i2c_read()" width=20cm
 *
 *<b>API Details:</b>
 * - The API attempts to read if the I2C bus is free, else it returns busy status #PAL_STATUS_I2C_BUSY<br>
 * - The bus is released only after the completion of reception or after completion of error handling.<br>
 * - The API invokes the upper layer handler with the respective event status as explained below.
 *   - #PAL_I2C_EVENT_BUSY when I2C bus in busy state
 *   - #PAL_I2C_EVENT_ERROR when API fails
 *   - #PAL_I2C_EVENT_SUCCESS when operation is successfully completed asynchronously
 *<br>
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t p_i2c_context must not be NULL.<br>
 * - The upper_layer_event_handler must be initialized in the p_i2c_context before invoking the API.<br>
 *
 *<b>Notes:</b><br>
 *  - Otherwise the below implementation has to be updated to handle different bitrates based on the input context.<br>
 *  - The caller of this API must take care of the guard time based on the slave's requirement.<br>
 *
 * \param[in]  p_i2c_context  pointer to the PAL i2c context #pal_i2c_t
 * \param[in]  p_data         Pointer to the data buffer to store the read data
 * \param[in]  length         Length of the data to be read
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the I2C read is invoked successfully
 * \retval  #PAL_STATUS_FAILURE  Returns when the I2C read fails.
 * \retval  #PAL_STATUS_I2C_BUSY Returns when the I2C bus is busy.
 */
pal_status_t pal_i2c_read(pal_i2c_t* p_i2c_context, uint8_t* p_data,
                          uint16_t length)
{
    pal_status_t status;
    app_event_handler_t upper_layer_handler =
            (app_event_handler_t)p_i2c_context->upper_layer_event_handler;


    if ((PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context)) && (p_i2c_context != NULL)) {
        int result;
        I2C_TransferSeq_TypeDef seq;

        seq.addr = (p_i2c_context->slave_address) << 1;
        seq.flags = I2C_FLAG_READ;

        seq.buf[0].len  = length;
        seq.buf[0].data = p_data;
        seq.buf[1].len  = 0;

        result = I2CSPM_Transfer(((i2c_ctx_t *)(p_i2c_context->p_i2c_hw_config))->sl_i2cspm_sensor, &seq);

        /*for(int count = 1; count < length; count++){
            seq.buf[0].data = p_data++;
            result = I2CSPM_Transfer(((i2c_ctx_t *)(p_i2c_context->p_i2c_hw_config))->sl_i2cspm_sensor, &seq);
            if(result != 0)
              break;
        }*/

        if (result == 0) {
            upper_layer_handler(p_i2c_context->upper_layer_ctx,
                                PAL_I2C_EVENT_SUCCESS);
            status = PAL_STATUS_SUCCESS;
        } else {
            upper_layer_handler(p_i2c_context->upper_layer_ctx,
                                PAL_I2C_EVENT_ERROR);
            status = PAL_STATUS_FAILURE;
        }

        pal_i2c_release((void *)p_i2c_context);
    } else {
        status = PAL_STATUS_I2C_BUSY;
        upper_layer_handler(p_i2c_context->upper_layer_ctx,
                            PAL_I2C_EVENT_BUSY);
    }

    return status;
}

/**
 * Sets the bitrate/speed(KHz) of I2C master.
 * <br>
 *
 *<b>API Details:</b>
 * - Sets the bitrate of I2C master if the I2C bus is free, else it returns busy status #PAL_STATUS_I2C_BUSY<br>
 * - The bus is released after the setting the bitrate.<br>
 * - This API must take care of setting the bitrate to I2C master's maximum supported value.
 * - Eg. In XMC4500, the maximum supported bitrate is 400 KHz. If the supplied bitrate is greater than 400KHz, the API will
 *   set the I2C master's bitrate to 400KHz.
 * - Use the #PAL_I2C_MASTER_MAX_BITRATE macro to specify the maximum supported bitrate value for the target platform.
 * - If upper_layer_event_handler is initialized, the upper layer handler is invoked with the respective event
 *   status listed below.
 *   - #PAL_I2C_EVENT_BUSY when I2C bus in busy state
 *   - #PAL_I2C_EVENT_ERROR when API fails to set the bit rate
 *   - #PAL_I2C_EVENT_SUCCESS when operation is successful
 *<br>
 *
 *<b>User Input:</b><br>
 * - The input #pal_i2c_t  p_i2c_context must not be NULL.<br>
 *
 * \param[in] p_i2c_context  Pointer to the pal i2c context
 * \param[in] bitrate        Bitrate to be used by i2c master in KHz
 *
 * \retval  #PAL_STATUS_SUCCESS  Returns when the setting of bitrate is successfully completed
 * \retval  #PAL_STATUS_FAILURE  Returns when the setting of bitrate fails.
 * \retval  #PAL_STATUS_I2C_BUSY Returns when the I2C bus is busy.
 */
pal_status_t pal_i2c_set_bitrate(const pal_i2c_t* p_i2c_context,
                                 uint16_t bitrate)
{
    /* not supporting config changes on run time */
    return PAL_STATUS_SUCCESS;
}

/**
* @}
*/
