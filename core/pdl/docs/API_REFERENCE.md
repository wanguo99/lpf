# PDL API 参考手册

## 卫星平台服务接口

### SatellitePDL_Init
初始化卫星平台服务。
```c
int32_t SatellitePDL_Init(const satellite_pdl_config_t *config);
```

### SatellitePDL_Deinit
关闭卫星平台服务。
```c
int32_t SatellitePDL_Deinit(void);
```

### SatellitePDL_SendTelemetry
发送遥测数据。
```c
int32_t SatellitePDL_SendTelemetry(uint32_t tm_id, const uint8_t *data, uint32_t len);
```

### SatellitePDL_GetStats
获取统计信息。
```c
int32_t SatellitePDL_GetStats(satellite_stats_t *stats);
```

## BMC载荷服务接口

### BMCPDL_Init
初始化BMC载荷服务。
```c
int32_t BMCPDL_Init(const bmc_pdl_config_t *config);
```

### BMCPDL_Deinit
关闭BMC载荷服务。
```c
int32_t BMCPDL_Deinit(void);
```

### BMCPDL_PowerOn
开机。
```c
int32_t BMCPDL_PowerOn(void);
```

### BMCPDL_PowerOff
关机。
```c
int32_t BMCPDL_PowerOff(void);
```

### BMCPDL_Reset
重启。
```c
int32_t BMCPDL_Reset(void);
```

### BMCPDL_GetPowerStatus
获取电源状态。
```c
int32_t BMCPDL_GetPowerStatus(bmc_power_status_t *status);
```

## MCU外设服务接口

### MCUPDL_Init
初始化MCU外设服务。
```c
int32_t MCUPDL_Init(const mcu_pdl_config_t *config);
```

### MCUPDL_Deinit
关闭MCU外设服务。
```c
int32_t MCUPDL_Deinit(void);
```

### MCUPDL_GetVersion
获取MCU版本。
```c
int32_t MCUPDL_GetVersion(char *version, uint32_t size);
```

### MCUPDL_FirmwareUpgrade
固件升级。
```c
int32_t MCUPDL_FirmwareUpgrade(const char *firmware_path);
```

详细参数说明请参考头文件 `pdl_satellite.h`、`pdl_bmc.h`、`pdl_mcu.h`。
