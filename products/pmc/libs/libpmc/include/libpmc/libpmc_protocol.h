#ifndef LIBPMC_PROTOCOL_H
#define LIBPMC_PROTOCOL_H

#include "libpmc_ipc.h"

/* 注意：源文件需要先包含 osal.h，再包含本头文件
 * 示例：
 *   #include "osal.h"
 *   #include "libpmc_protocol.h"
 */

/* CAN帧ID定义 */
#define PMC_CAN_ID_TC_CMD          0x100  /* 遥控命令 */
#define PMC_CAN_ID_TM_REQUEST      0x200  /* 遥测请求 */
#define PMC_CAN_ID_TM_RESPONSE     0x201  /* 遥测应答 */
#define PMC_CAN_ID_HEARTBEAT       0x300  /* 心跳包 */

/* 遥控命令类型 */
typedef enum {
    PMC_TC_SERVER_POWER_ON = 0x01,      /* 服务器上电 */
    PMC_TC_SERVER_POWER_OFF,            /* 服务器下电 */
    PMC_TC_SERVER_RESET,                /* 服务器复位 */
    PMC_TC_MCU_RESET,                   /* MCU复位 */
    PMC_TC_FPGA_RESET,                  /* FPGA复位 */
    PMC_TC_MAX
} pmc_tc_type_t;

/* 遥测类型 */
typedef enum {
    PMC_TM_SERVER_STATUS = 0x01,        /* 服务器状态 */
    PMC_TM_CPU_TEMP,                    /* CPU温度 */
    PMC_TM_BOARD_TEMP,                  /* 板卡温度 */
    PMC_TM_VOLTAGE_54V,                 /* 54V电压 */
    PMC_TM_VOLTAGE_12V,                 /* 12V电压 */
    PMC_TM_MCU_STATUS,                  /* MCU状态 */
    PMC_TM_FPGA_STATUS,                 /* FPGA状态 */
    PMC_TM_MAX
} pmc_tm_type_t;

/* CAN帧结构 */
typedef struct {
    uint32_t can_id;                      /* CAN ID */
    uint8_t data[0x8];                      /* 数据 */
    uint8_t dlc;                          /* 数据长度 */
} pmc_can_frame_t;

/* 遥控命令帧 */
typedef struct {
    pmc_tc_type_t cmd_type;             /* 命令类型 */
    uint32_t param;                       /* 参数 */
} pmc_tc_frame_t;

/* 遥测请求帧 */
typedef struct {
    pmc_tm_type_t tm_type;              /* 遥测类型 */
} pmc_tm_request_t;

/* 遥测应答帧 */
typedef struct {
    pmc_tm_type_t tm_type;              /* 遥测类型 */
    uint8_t data[PMC_TM_MAX_DATA_SIZE];   /* 遥测数据 */
    uint32_t data_size;                   /* 数据长度 */
    uint8_t freshness;                    /* 新鲜度 */
} pmc_tm_response_t;

/* 心跳包 */
typedef struct {
    uint32_t sequence;                    /* 序列号 */
    uint32_t status;                      /* 状态 */
} pmc_heartbeat_t;

#endif /* LIBPMC_PROTOCOL_H */
