# HWID（硬件ID）设计方案

## 设计原则

1. **不可变性**：HWID 一旦写入不应更改（通常写入 EEPROM/OTP）
2. **唯一性**：每块板子都有唯一的序列号
3. **可扩展性**：支持格式演进，预留扩展字段
4. **可验证性**：包含魔数和校验码
5. **可读性**：字段含义清晰，便于调试

## 推荐设计方案 1：紧凑型（20 字节）

```c
/**
 * @brief HWID 结构体 - 紧凑型
 * 
 * 特点：占用空间小（20字节），适合存储在 EEPROM/OTP 中
 * 限制：字段位宽受限，序列号较短
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;             /* 魔数：0x48574944 ('HWID') - 用于验证有效性 */
    uint8_t  format_version;    /* HWID 格式版本：0x01 - 支持未来扩展 */
    uint8_t  vendor_id;         /* 厂商ID：0x01=公司A, 0x02=公司B */
    
    uint16_t product_id;        /* 产品ID：0x0001=H200, 0x0002=H300 */
    uint16_t project_id;        /* 项目ID：0x0001=100P, 0x0002=200P */
    
    uint8_t  board_type;        /* 板卡类型：0x01=主板, 0x02=扩展板 */
    uint8_t  hw_revision;       /* 硬件版本：0x01=V1.0, 0x11=V1.1, 0x20=V2.0 */
    
    uint32_t serial_number;     /* 序列号：0x00000001-0xFFFFFFFF */
    
    uint16_t manufacture_date;  /* 生产日期：(year-2020)*512 + month*32 + day */
    uint16_t crc16;             /* CRC16 校验：校验前 18 字节 */
} pdl_hwid_t;

/* Magic 定义 */
#define PDL_HWID_MAGIC          0x48574944  /* 'HWID' - ASCII 编码 */
#define PDL_HWID_INVALID        0x00000000  /* 无效 HWID */
#define PDL_HWID_UNKNOWN        0xFFFFFFFF  /* 未知 HWID（已废弃，使用 magic=0 代替） */
```

### 字段说明

| 字段 | 大小 | 说明 | 示例 |
|------|------|------|------|
| magic | 4B | 魔数 0x48574944 ('HWID')，用于验证 | 0x48574944 |
| format_version | 1B | HWID 格式版本，支持演进 | 0x01 |
| vendor_id | 1B | 厂商ID | 0x01 |
| product_id | 2B | 产品ID | 0x0001=H200 |
| project_id | 2B | 项目ID | 0x0001=100P |
| board_type | 1B | 板卡类型 | 0x01=主板 |
| hw_revision | 1B | 硬件版本 | 0x11=V1.1 |
| serial_number | 4B | 序列号（唯一） | 0x00012345 |
| manufacture_date | 2B | 生产日期（压缩格式） | 0x0821=2024年1月1日 |
| crc16 | 2B | CRC16 校验 | - |

**总大小：20 字节**

### 生产日期编码

```c
/* 编码：(year-2020)*512 + month*32 + day */
/* 范围：2020-2147年，1-12月，1-31日 */
uint16_t encode_date(uint16_t year, uint8_t month, uint8_t day) {
    return ((year - 2020) * 512) + (month * 32) + day;
}

void decode_date(uint16_t encoded, uint16_t *year, uint8_t *month, uint8_t *day) {
    *year = 2020 + (encoded / 512);
    *month = (encoded % 512) / 32;
    *day = encoded % 32;
}
```

## 推荐设计方案 2：完整型（32 字节）

```c
/**
 * @brief HWID 结构体 - 完整型
 * 
 * 特点：字段完整，便于扩展，便于调试
 * 适用：存储空间充足的场景
 */
typedef struct __attribute__((packed)) {
    /* ===== 标识区（8字节）===== */
    uint32_t magic;             /* 魔数：0x48574944 ('HWID') */
    uint8_t  format_version;    /* HWID 格式版本 */
    uint8_t  vendor_id;         /* 厂商ID */
    uint16_t reserved1;         /* 预留 */
    
    /* ===== 产品信息（8字节）===== */
    uint16_t product_id;        /* 产品ID */
    uint16_t project_id;        /* 项目ID */
    uint8_t  board_type;        /* 板卡类型 */
    uint8_t  hw_major;          /* 硬件主版本 */
    uint8_t  hw_minor;          /* 硬件次版本 */
    uint8_t  hw_revision;       /* 硬件修订版本 */
    
    /* ===== 制造信息（12字节）===== */
    uint64_t serial_number;     /* 序列号（64位，足够大） */
    uint32_t manufacture_date;  /* 生产日期：Unix timestamp */
    
    /* ===== 扩展和校验（4字节）===== */
    uint16_t reserved2;         /* 预留扩展 */
    uint16_t crc16;             /* CRC16 校验：校验前 30 字节 */
} pdl_hwid_extended_t;

/* Magic 定义 */
#define PDL_HWID_MAGIC          0x48574944  /* 'HWID' - 紧凑型和完整型统一使用 */
```

## 推荐设计方案 3：灵活型（可变长）

```c
/**
 * @brief HWID 结构体 - 灵活型
 * 
 * 特点：支持可选字段，支持未来扩展
 * 使用 TLV（Type-Length-Value）编码
 */

/* HWID 字段类型 */
typedef enum {
    HWID_FIELD_MAGIC         = 0x01,  /* 魔数 */
    HWID_FIELD_FORMAT_VER    = 0x02,  /* 格式版本 */
    HWID_FIELD_VENDOR        = 0x03,  /* 厂商ID */
    HWID_FIELD_PRODUCT       = 0x04,  /* 产品ID */
    HWID_FIELD_PROJECT       = 0x05,  /* 项目ID */
    HWID_FIELD_BOARD_TYPE    = 0x06,  /* 板卡类型 */
    HWID_FIELD_HW_VERSION    = 0x07,  /* 硬件版本 */
    HWID_FIELD_SERIAL        = 0x08,  /* 序列号 */
    HWID_FIELD_MFG_DATE      = 0x09,  /* 生产日期 */
    HWID_FIELD_MAC_ADDR      = 0x0A,  /* MAC 地址 */
    HWID_FIELD_RESERVED      = 0xFE,  /* 预留 */
    HWID_FIELD_CRC           = 0xFF,  /* CRC 校验 */
} hwid_field_type_t;

/* TLV 格式 */
typedef struct {
    uint8_t type;      /* 字段类型 */
    uint8_t length;    /* 字段长度 */
    uint8_t value[];   /* 字段值（柔性数组） */
} hwid_tlv_t;
```

## 辅助函数设计

```c
/**
 * @brief 验证 HWID 有效性
 */
bool PDL_MISC_ValidateHWID(const pdl_hwid_t *hwid);

/**
 * @brief 计算 HWID 的 CRC
 */
uint16_t PDL_MISC_CalculateHWID_CRC(const pdl_hwid_t *hwid);

/**
 * @brief 比较两个 HWID 是否匹配
 * 
 * @param hwid1 HWID 1
 * @param hwid2 HWID 2
 * @param mask  比较掩码（指定哪些字段需要比较）
 * @return true=匹配，false=不匹配
 */
bool PDL_MISC_CompareHWID(const pdl_hwid_t *hwid1, 
                          const pdl_hwid_t *hwid2,
                          uint32_t mask);

/**
 * @brief 格式化 HWID 为字符串（便于调试）
 * 
 * 格式：HW:V1-0001-0001-01-1.1-SN:12345678-MFG:2024-01-01
 */
int32_t PDL_MISC_FormatHWID(const pdl_hwid_t *hwid, 
                            char *buf, 
                            size_t buf_size);

/**
 * @brief 从字符串解析 HWID
 */
int32_t PDL_MISC_ParseHWID(const char *str, pdl_hwid_t *hwid);
```

## HWID 存储位置选择

### 1. EEPROM（推荐）
- **优点**：可靠、容量适中（1KB-8KB）、可重写（生产时写入）
- **缺点**：需要外部芯片
- **适用**：生产环境

### 2. OTP（One-Time Programmable）
- **优点**：安全、不可篡改
- **缺点**：只能写入一次，无法修正错误
- **适用**：高安全要求场景

### 3. Flash（配置区）
- **优点**：无需额外硬件、容量大
- **缺点**：可能被意外擦除
- **适用**：开发测试环境

### 4. 设备树（Linux）
- **优点**：灵活、便于开发
- **缺点**：依赖操作系统
- **适用**：Linux 平台

## 使用示例

```c
/* 读取 HWID */
pdl_hwid_t hwid;
if (PDL_MISC_GetHWID(&hwid) == OSAL_SUCCESS) {
    /* 验证 HWID */
    if (PDL_MISC_ValidateHWID(&hwid)) {
        /* 打印 HWID */
        char buf[128];
        PDL_MISC_FormatHWID(&hwid, buf, sizeof(buf));
        LOG_INFO("HWID: %s", buf);
        
        /* 根据 HWID 加载配置 */
        PCONFIG_LoadByHWID();
    } else {
        LOG_ERROR("Invalid HWID!");
    }
}
```

## 配置匹配示例

```c
/* 配置 1：支持 H200 V1.0-V1.2 */
static const pdl_hwid_t hwid_list_h200_v1[] = {
    {.magic=0x4857, .product_id=0x0001, .hw_revision=0x10, ...},  /* V1.0 */
    {.magic=0x4857, .product_id=0x0001, .hw_revision=0x11, ...},  /* V1.1 */
    {.magic=0x4857, .product_id=0x0001, .hw_revision=0x12, ...},  /* V1.2 */
};

/* 配置 2：支持 H200 V2.x */
static const pdl_hwid_t hwid_list_h200_v2[] = {
    {.magic=0x4857, .product_id=0x0001, .hw_revision=0x20, ...},  /* V2.0 */
    {.magic=0x4857, .product_id=0x0001, .hw_revision=0x21, ...},  /* V2.1 */
};

/* 或者使用掩码匹配 */
typedef struct {
    pdl_hwid_t hwid;     /* HWID 模板 */
    uint32_t mask;       /* 比较掩码 */
} hwid_pattern_t;

static const hwid_pattern_t pattern_h200_v1 = {
    .hwid = {
        .product_id = 0x0001,
        .hw_revision = 0x10,
    },
    .mask = HWID_MASK_PRODUCT | HWID_MASK_HW_MAJOR,  /* 只比较产品ID和主版本 */
};
```

## 推荐方案总结

**推荐使用方案 1（紧凑型）**，原因：
1. ✅ 16 字节大小适中，适合 EEPROM 存储
2. ✅ 字段完整，包含所有必要信息
3. ✅ 支持 40 亿序列号（足够使用）
4. ✅ 生产日期压缩编码，节省空间
5. ✅ 包含 CRC 校验，保证数据完整性
6. ✅ 预留格式版本，支持未来扩展

**何时使用方案 2（完整型）**：
- 存储空间充足（Flash 存储）
- 需要更大的序列号空间（64位）
- 需要 Unix timestamp 格式的日期
- 需要更多预留字段

**何时使用方案 3（灵活型）**：
- 不同产品线字段差异大
- 需要频繁添加新字段
- 需要向后兼容
