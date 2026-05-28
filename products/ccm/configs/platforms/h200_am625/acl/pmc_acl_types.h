/**
 * @file pmc_acl_types.h
 * @brief PMC 项目 ACL 功能 ID 定义
 * @note 为 PMC 项目定义特定的遥控和遥测功能 ID 别名
 */

#ifndef PMC_ACL_TYPES_H
#define PMC_ACL_TYPES_H

#include "acl/acl_tc.h"
#include "acl/acl_tm.h"

/* 遥控命令别名 - 映射到通用 ACL 定义 */
#define TC_SERVER_POWER_ON          TC_POWER_ON
#define TC_SERVER_POWER_OFF         TC_POWER_OFF
#define TC_SERVER_POWER_RESET       TC_POWER_RESET
#define TC_SERVER_POWER_CYCLE       TC_POWER_CYCLE
#define TC_SERVER_SOFT_RESET        TC_SOFT_RESET
#define TC_SERVER_HARD_RESET        TC_HARD_RESET

/* 遥测别名 - 映射到通用 ACL 定义 */
#define TM_SERVER_CPU_TEMP          TM_CPU_TEMP
#define TM_SERVER_BOARD_TEMP        TM_BOARD_TEMP
#define TM_SERVER_FAN_SPEED         TM_FAN_SPEED
#define TM_SERVER_VOLTAGE_12V       TM_VOLTAGE_12V
#define TM_SERVER_VOLTAGE_5V        TM_VOLTAGE_5V
#define TM_SERVER_VOLTAGE_3V3       TM_VOLTAGE_3V3
#define TM_SERVER_CURRENT           TM_CURRENT
#define TM_SERVER_POWER_STATUS      TM_POWER_STATUS
#define TM_MCU_VOLTAGE              TM_VOLTAGE_3V3  /* MCU 电压使用 3.3V 遥测 */

/**
 * @brief 遥控命令对遥测的失效映射
 * @note 定义遥控命令执行后，哪些遥测需要标记为 STALE 并重新采集
 */
typedef struct {
    uint32_t tc_function;          /* 遥控功能 ID */
    uint32_t affected_tm[16];      /* 受影响的遥测 ID 数组（最多 16 个）*/
    uint32_t affected_count;       /* 受影响的遥测数量 */
} tc_tm_invalidation_map_t;

#endif /* PMC_ACL_TYPES_H */
