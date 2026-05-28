# PCL API 参考手册

## 初始化和注册

### HW_Config_Init
初始化配置库。
```c
int32_t HW_Config_Init(void);
```

### HW_Config_RegisterAll
注册所有配置。
```c
int32_t HW_Config_RegisterAll(void);
```

### HW_Config_SelectDefault
选择默认配置。
```c
const pcl_board_config_t* HW_Config_SelectDefault(void);
```

### HW_Config_GetBoard
获取当前板级配置。
```c
const pcl_board_config_t* HW_Config_GetBoard(void);
```

### HW_Config_Print
打印配置信息。
```c
void HW_Config_Print(const pcl_board_config_t *board);
```

## 外设配置查询

### XCONFIG_HW_FindMCU
查找MCU外设配置。
```c
const pcl_mcu_cfg_t* XCONFIG_HW_FindMCU(const pcl_board_config_t *board,
                                            const char *name);
```

### XCONFIG_HW_FindBMC
查找BMC外设配置。
```c
const pcl_bmc_cfg_t* XCONFIG_HW_FindBMC(const pcl_board_config_t *board,
                                            const char *name);
```

### XCONFIG_HW_FindSatellite
查找卫星平台接口配置。
```c
const pcl_satellite_cfg_t* XCONFIG_HW_FindSatellite(const pcl_board_config_t *board,
                                                        const char *name);
```

### XCONFIG_HW_FindSensor
查找传感器外设配置。
```c
const pcl_sensor_cfg_t* XCONFIG_HW_FindSensor(const pcl_board_config_t *board,
                                                  const char *name);
```

### XCONFIG_HW_FindStorage
查找存储设备配置。
```c
const pcl_storage_cfg_t* XCONFIG_HW_FindStorage(const pcl_board_config_t *board,
                                                    const char *name);
```

## 配置数据结构

详细的数据结构定义请参考头文件：
- `pcl_mcu.h` - MCU外设配置
- `pcl_bmc.h` - BMC外设配置
- `pcl_satellite.h` - 卫星平台配置
- `pcl_sensor.h` - 传感器配置
- `pcl_storage.h` - 存储设备配置

完整的API文档请参考 [完整README](README.md)。
