#include <stdio.h>
#include <string.h>
#include "hal_can.h"
#include "osal.h"

static void print_can_frame(const hal_can_frame_t *frame)
{
    printf("  ID: 0x%03X\n", frame->can_id);
    printf("  DLC: %d\n", frame->can_dlc);
    printf("  Data: ");
    for (int i = 0; i < frame->can_dlc; i++) {
        printf("%02X ", frame->data[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    const char *device = "can0";

    if (argc > 1) {
        device = argv[1];
    }

    printf("=================================\n");
    printf("  HAL CAN Example\n");
    printf("=================================\n\n");

    printf("Opening CAN device: %s\n", device);

    // 配置 CAN
    hal_can_config_t config = {
        .bitrate = 500000,  // 500 kbps
        .mode = 0,          // Normal mode
    };

    // 打开 CAN 设备
    void *can_handle = HAL_CAN_Open(device, &config);
    if (!can_handle) {
        printf("Failed to open CAN device: %s\n", device);
        printf("\nTroubleshooting:\n");
        printf("  1. Check if CAN interface exists: ip link show\n");
        printf("  2. Configure CAN: sudo ip link set %s type can bitrate 500000\n", device);
        printf("  3. Bring up CAN: sudo ip link set %s up\n", device);
        printf("  4. Or use virtual CAN: sudo modprobe vcan && sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0\n");
        return -1;
    }

    printf("CAN device opened successfully\n\n");

    // 发送 CAN 消息
    printf("Sending CAN message...\n");
    hal_can_frame_t tx_frame = {
        .can_id = 0x123,
        .can_dlc = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
    };

    print_can_frame(&tx_frame);

    int32_t ret = HAL_CAN_Write(can_handle, &tx_frame);
    if (ret < 0) {
        printf("Failed to send CAN message\n");
    } else {
        printf("Message sent successfully\n\n");
    }

    // 接收 CAN 消息
    printf("Waiting for CAN messages (5 seconds)...\n");
    printf("Tip: Use 'cansend %s 456#AABBCCDDEEFF0011' to send a test message\n\n", device);

    hal_can_frame_t rx_frame;
    ret = HAL_CAN_Read(can_handle, &rx_frame, 5000);  // 超时 5 秒

    if (ret > 0) {
        printf("Received CAN message:\n");
        print_can_frame(&rx_frame);
    } else if (ret == 0) {
        printf("Timeout: No message received\n");
    } else {
        printf("Failed to receive CAN message\n");
    }

    // 关闭 CAN 设备
    HAL_CAN_Close(can_handle);
    printf("\nCAN device closed\n");

    printf("\nNext steps:\n");
    printf("  1. Modify CAN ID and data\n");
    printf("  2. Implement continuous receive loop\n");
    printf("  3. Add CAN filter\n");
    printf("  4. Try with real CAN hardware\n");

    return 0;
}
