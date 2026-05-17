/************************************************************************
 * HAL层 - SPI驱动 (macOS 打桩实现)
 *
 * 说明：
 * - 这是 macOS 平台的打桩实现，仅用于编译
 * - 所有函数返回 OSAL_ERR_NOT_IMPLEMENTED
 * - 实际硬件访问需要在 Linux 平台上运行
 ************************************************************************/

#include "hal_spi.h"
#include "osal.h"

int32_t HAL_SPI_Open(const hal_spi_config_t *config, hal_spi_handle_t *handle)
{
    (void)config;
    (void)handle;

    LOG_WARN("HAL_SPI", "SPI driver not implemented on macOS (stub only)");
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_SPI_Close(hal_spi_handle_t handle)
{
    (void)handle;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_SPI_Write(hal_spi_handle_t handle, const uint8_t *buffer, uint32_t size)
{
    (void)handle;
    (void)buffer;
    (void)size;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_SPI_Read(hal_spi_handle_t handle, uint8_t *buffer, uint32_t size)
{
    (void)handle;
    (void)buffer;
    (void)size;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_SPI_Transfer(hal_spi_handle_t handle, const uint8_t *tx_buffer,
                         uint8_t *rx_buffer, uint32_t size)
{
    (void)handle;
    (void)tx_buffer;
    (void)rx_buffer;
    (void)size;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_SPI_TransferMulti(hal_spi_handle_t handle, spi_transfer_t *transfers, uint32_t num)
{
    (void)handle;
    (void)transfers;
    (void)num;
    return OSAL_ERR_NOT_IMPLEMENTED;
}

int32_t HAL_SPI_SetConfig(hal_spi_handle_t handle, const hal_spi_config_t *config)
{
    (void)handle;
    (void)config;
    return OSAL_ERR_NOT_IMPLEMENTED;
}
