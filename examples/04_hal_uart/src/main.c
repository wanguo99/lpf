#include <stdio.h>
#include <string.h>
#include "hal_serial.h"
#include "osal/osal.h"

int main(int argc, char *argv[])
{
    const char *device = "/dev/ttyUSB0";

    if (argc > 1) {
        device = argv[1];
    }

    printf("=================================\n");
    printf("  HAL UART Example\n");
    printf("=================================\n\n");

    printf("Opening UART device: %s\n", device);

    // 配置串口
    hal_serial_config_t config = {
        .baudrate = 115200,
        .databits = 8,
        .stopbits = 1,
        .parity = 'N',  // No parity
    };

    // 打开串口设备
    void *uart_handle = HAL_Serial_Open(device, &config);
    if (!uart_handle) {
        printf("Failed to open UART device: %s\n", device);
        printf("\nTroubleshooting:\n");
        printf("  1. Check if device exists: ls -l %s\n", device);
        printf("  2. Check permissions: sudo chmod 666 %s\n", device);
        printf("  3. Add user to dialout group: sudo usermod -a -G dialout $USER\n");
        printf("  4. List all serial devices: ls /dev/tty*\n");
        return -1;
    }

    printf("UART device opened successfully\n");
    printf("Configuration: %d 8N1\n\n", config.baudrate);

    // 发送数据
    const char *message = "Hello, UART!\n";
    printf("Sending data: %s", message);

    int32_t sent = HAL_Serial_Write(uart_handle,
                                     (const uint8_t *)message,
                                     strlen(message));
    if (sent < 0) {
        printf("Failed to send data\n");
    } else {
        printf("Data sent: %d bytes\n\n", sent);
    }

    // 接收数据
    printf("Waiting for data (5 seconds)...\n");
    printf("Tip: Connect another terminal and send data\n");
    printf("     echo 'Hello back!' > %s\n\n", device);

    uint8_t buffer[256];
    int32_t received = HAL_Serial_Read(uart_handle, buffer, sizeof(buffer) - 1);

    if (received > 0) {
        buffer[received] = '\0';  // 添加字符串结束符
        printf("Received %d bytes: %s\n", received, buffer);
    } else if (received == 0) {
        printf("Timeout: No data received\n");
    } else {
        printf("Failed to receive data\n");
    }

    // 关闭串口设备
    HAL_Serial_Close(uart_handle);
    printf("\nUART device closed\n");

    printf("\nNext steps:\n");
    printf("  1. Implement echo server\n");
    printf("  2. Try different baud rates\n");
    printf("  3. Send binary data\n");
    printf("  4. Add error handling\n");

    return 0;
}
