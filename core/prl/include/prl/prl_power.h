/**
 * @file prl_power.h
 * @brief POWER Device Protocol
 */

#ifndef PRL_PRL_POWER_H
#define PRL_PRL_POWER_H

#include "osal/osal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* POWER Message Types */
typedef enum {
    PRL_POWER_MSG_HEARTBEAT     = 0x01,
    PRL_POWER_MSG_POWER_ON      = 0x02,
    PRL_POWER_MSG_POWER_OFF     = 0x03,
    PRL_POWER_MSG_VOLTAGE_QUERY = 0x04,
    PRL_POWER_MSG_CURRENT_QUERY = 0x05,
    PRL_POWER_MSG_TEMP_QUERY    = 0x06,
    PRL_POWER_MSG_STATUS_REPORT = 0x07,
    PRL_POWER_MSG_ALARM         = 0x08
} prl_power_msg_type_t;

/* POWER Alarm Types */
typedef enum {
    PRL_POWER_ALARM_OVER_VOLTAGE    = 0x01,
    PRL_POWER_ALARM_UNDER_VOLTAGE   = 0x02,
    PRL_POWER_ALARM_OVER_CURRENT    = 0x03,
    PRL_POWER_ALARM_OVER_TEMP       = 0x04,
    PRL_POWER_ALARM_SHORT_CIRCUIT   = 0x05,
    PRL_POWER_ALARM_POWER_FAIL      = 0x06
} prl_power_alarm_type_t;

/* POWER Status */
typedef struct {
    uint8_t  enabled;
    uint8_t  fault;
    uint16_t voltage;
    uint16_t current;
    uint16_t temperature;
} prl_power_status_t;

#ifdef __cplusplus
}
#endif

#endif /* PRL_PRL_POWER_H */
