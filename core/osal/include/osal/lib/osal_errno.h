/************************************************************************
 * OSAL Errno API
 *
 * 错误码定义和错误处理API
 *
 * 提供：
 * - POSIX errno常量定义（OSAL_EPERM, OSAL_ENOENT等）
 * - 语义化错误码别名（OSAL_ERR_INVALID_POINTER等）
 * - OSAL特定错误码（OSAL_ERR_ADDRESS_MISALIGNED等）
 * - errno访问函数
 * - 状态码转字符串函数
 ************************************************************************/

#ifndef OSAL_ERRNO_H
#define OSAL_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

/*===========================================================================
 * 状态码类型
 *===========================================================================*/

/**
 * @brief OSAL状态码类型
 *
 * 成功返回 OSAL_SUCCESS (0)
 * 错误返回正数错误码
 */
typedef int32_t osal_status_t;

/*
 * 成功状态码
 */
#define OSAL_SUCCESS                    0x0

/*===========================================================================
 * POSIX errno 常量定义
 * 按照 Linux errno.h 的顺序定义，提供跨平台的统一错误码
 *===========================================================================*/

#define OSAL_EPERM           0x1   /* 操作不允许 */
#define OSAL_ENOENT          0x2   /* 文件或目录不存在 */
#define OSAL_ESRCH           0x3   /* 进程不存在 */
#define OSAL_EINTR           0x4   /* 系统调用被中断 */
#define OSAL_EIO             0x5   /* I/O错误 */
#define OSAL_ENXIO           0x6   /* 设备或地址不存在 */
#define OSAL_E2BIG           0x7   /* 参数列表过长 */
#define OSAL_ENOEXEC         0x8   /* 执行格式错误 */
#define OSAL_EBADF           0x9   /* 文件描述符错误 */
#define OSAL_ECHILD          0xA  /* 子进程不存在 */
#define OSAL_EAGAIN          0xB  /* 资源暂时不可用 */
#define OSAL_ENOMEM          0xC  /* 内存不足 */
#define OSAL_EACCES          0xD  /* 权限不足 */
#define OSAL_EFAULT          0xE  /* 地址错误 */
#define OSAL_ENOTBLK         0xF  /* 需要块设备 */
#define OSAL_EBUSY           0x10  /* 设备或资源忙 */
#define OSAL_EEXIST          0x11  /* 文件已存在 */
#define OSAL_EXDEV           0x12  /* 跨设备链接 */
#define OSAL_ENODEV          0x13  /* 设备不存在 */
#define OSAL_ENOTDIR         0x14  /* 不是目录 */
#define OSAL_EISDIR          0x15  /* 是目录 */
#define OSAL_EINVAL          0x16  /* 参数无效 */
#define OSAL_ENFILE          0x17  /* 系统打开文件过多 */
#define OSAL_EMFILE          0x18  /* 进程打开文件过多 */
#define OSAL_ENOTTY          0x19  /* 不是终端设备 */
#define OSAL_ETXTBSY         0x1A  /* 文本文件忙 */
#define OSAL_EFBIG           0x1B  /* 文件过大 */
#define OSAL_ENOSPC          0x1C  /* 设备空间不足 */
#define OSAL_ESPIPE          0x1D  /* 非法seek */
#define OSAL_EROFS           0x1E  /* 只读文件系统 */
#define OSAL_EMLINK          0x1F  /* 链接过多 */
#define OSAL_EPIPE           0x20  /* 管道破裂 */
#define OSAL_EDOM            0x21  /* 数学参数超出范围 */
#define OSAL_ERANGE          0x22  /* 数学结果无法表示 */
#define OSAL_EDEADLK         0x23  /* 资源死锁 */
#define OSAL_ENAMETOOLONG    0x24  /* 文件名过长 */
#define OSAL_ENOLCK          0x25  /* 锁不可用 */
#define OSAL_ENOSYS          0x26  /* 功能未实现 */
#define OSAL_ENOTEMPTY       0x27  /* 目录非空 */
#define OSAL_ELOOP           0x28  /* 符号链接层次过多 */
#define OSAL_EWOULDBLOCK     OSAL_EAGAIN  /* 操作会阻塞 */
#define OSAL_ENOMSG          0x2A  /* 无消息 */
#define OSAL_EIDRM           0x2B  /* 标识符已删除 */
#define OSAL_ECHRNG          0x2C  /* 通道号超出范围 */
#define OSAL_EL2NSYNC        0x2D  /* 2级不同步 */
#define OSAL_EL3HLT          0x2E  /* 3级停止 */
#define OSAL_EL3RST          0x2F  /* 3级复位 */
#define OSAL_ELNRNG          0x30  /* 链接号超出范围 */
#define OSAL_EUNATCH         0x31  /* 协议驱动未连接 */
#define OSAL_ENOCSI          0x32  /* 无CSI结构 */
#define OSAL_EL2HLT          0x33  /* 2级停止 */
#define OSAL_EBADE           0x34  /* 无效交换 */
#define OSAL_EBADR           0x35  /* 无效请求描述符 */
#define OSAL_EXFULL          0x36  /* 交换满 */
#define OSAL_ENOANO          0x37  /* 无阳极 */
#define OSAL_EBADRQC         0x38  /* 无效请求码 */
#define OSAL_EBADSLT         0x39  /* 无效槽 */
#define OSAL_EDEADLOCK       OSAL_EDEADLK
#define OSAL_EBFONT          0x3B  /* 字体文件格式错误 */
#define OSAL_ENOSTR          0x3C  /* 不是流设备 */
#define OSAL_ENODATA         0x3D  /* 无数据 */
#define OSAL_ETIME           0x3E  /* 定时器超时 */
#define OSAL_ENOSR           0x3F  /* 流资源不足 */
#define OSAL_ENONET          0x40  /* 机器不在网络上 */
#define OSAL_ENOPKG          0x41  /* 包未安装 */
#define OSAL_EREMOTE         0x42  /* 对象是远程的 */
#define OSAL_ENOLINK         0x43  /* 链接已断开 */
#define OSAL_EADV            0x44  /* 通告错误 */
#define OSAL_ESRMNT          0x45  /* Srmount错误 */
#define OSAL_ECOMM           0x46  /* 发送时通信错误 */
#define OSAL_EPROTO          0x47  /* 协议错误 */
#define OSAL_EMULTIHOP       0x48  /* 多跳尝试 */
#define OSAL_EDOTDOT         0x49  /* RFS特定错误 */
#define OSAL_EBADMSG         0x4A  /* 消息错误 */
#define OSAL_EOVERFLOW       0x4B  /* 值对数据类型过大 */
#define OSAL_ENOTUNIQ        0x4C  /* 名称在网络上不唯一 */
#define OSAL_EBADFD          0x4D  /* 文件描述符状态错误 */
#define OSAL_EREMCHG         0x4E  /* 远程地址改变 */
#define OSAL_ELIBACC         0x4F  /* 无法访问共享库 */
#define OSAL_ELIBBAD         0x50  /* 共享库损坏 */
#define OSAL_ELIBSCN         0x51  /* .lib节损坏 */
#define OSAL_ELIBMAX         0x52  /* 链接共享库过多 */
#define OSAL_ELIBEXEC        0x53  /* 无法直接执行共享库 */
#define OSAL_EILSEQ          0x54  /* 非法字节序列 */
#define OSAL_ERESTART        0x55  /* 应重启系统调用 */
#define OSAL_ESTRPIPE        0x56  /* 流管道错误 */
#define OSAL_EUSERS          0x57  /* 用户过多 */
#define OSAL_ENOTSOCK        0x58  /* 非socket操作 */
#define OSAL_EDESTADDRREQ    0x59  /* 需要目标地址 */
#define OSAL_EMSGSIZE        0x5A  /* 消息过长 */
#define OSAL_EPROTOTYPE      0x5B  /* socket协议类型错误 */
#define OSAL_ENOPROTOOPT     0x5C  /* 协议不可用 */
#define OSAL_EPROTONOSUPPORT 0x5D  /* 协议不支持 */
#define OSAL_ESOCKTNOSUPPORT 0x5E  /* socket类型不支持 */
#define OSAL_EOPNOTSUPP      0x5F  /* 操作不支持 */
#define OSAL_EPFNOSUPPORT    0x60  /* 协议族不支持 */
#define OSAL_EAFNOSUPPORT    0x61  /* 地址族不支持 */
#define OSAL_EADDRINUSE      0x62  /* 地址已使用 */
#define OSAL_EADDRNOTAVAIL   0x63  /* 地址不可用 */
#define OSAL_ENETDOWN        0x64 /* 网络已关闭 */
#define OSAL_ENETUNREACH     0x65 /* 网络不可达 */
#define OSAL_ENETRESET       0x66 /* 网络连接复位 */
#define OSAL_ECONNABORTED    0x67 /* 连接中止 */
#define OSAL_ECONNRESET      0x68 /* 连接复位 */
#define OSAL_ENOBUFS         0x69 /* 缓冲区空间不足 */
#define OSAL_EISCONN         0x6A /* 已连接 */
#define OSAL_ENOTCONN        0x6B /* 未连接 */
#define OSAL_ESHUTDOWN       0x6C /* 传输端点已关闭 */
#define OSAL_ETOOMANYREFS    0x6D /* 引用过多 */
#define OSAL_ETIMEDOUT       0x6E /* 连接超时 */
#define OSAL_ECONNREFUSED    0x6F /* 连接被拒绝 */
#define OSAL_EHOSTDOWN       0x70 /* 主机已关闭 */
#define OSAL_EHOSTUNREACH    0x71 /* 主机不可达 */
#define OSAL_EALREADY        0x72 /* 操作已在进行 */
#define OSAL_EINPROGRESS     0x73 /* 操作正在进行 */
#define OSAL_ESTALE          0x74 /* 陈旧的文件句柄 */
#define OSAL_EUCLEAN         0x75 /* 结构需要清理 */
#define OSAL_ENOTNAM         0x76 /* 不是XENIX命名文件 */
#define OSAL_ENAVAIL         0x77 /* 无XENIX信号量 */
#define OSAL_EISNAM          0x78 /* 是命名文件 */
#define OSAL_EREMOTEIO       0x79 /* 远程I/O错误 */
#define OSAL_EDQUOT          0x7A /* 超出配额 */
#define OSAL_ENOMEDIUM       0x7B /* 无介质 */
#define OSAL_EMEDIUMTYPE     0x7C /* 介质类型错误 */
#define OSAL_ECANCELED       0x7D /* 操作已取消 */
#define OSAL_ENOKEY          0x7E /* 所需密钥不可用 */
#define OSAL_EKEYEXPIRED     0x7F /* 密钥已过期 */
#define OSAL_EKEYREVOKED     0x80 /* 密钥已撤销 */
#define OSAL_EKEYREJECTED    0x81 /* 密钥被服务拒绝 */
#define OSAL_EOWNERDEAD      0x82 /* 所有者死亡 */
#define OSAL_ENOTRECOVERABLE 0x83 /* 状态不可恢复 */
#define OSAL_ERFKILL         0x84 /* 因RF-kill而无法操作 */
#define OSAL_EHWPOISON       0x85 /* 内存页硬件错误 */

/*===========================================================================
 * 语义化错误码别名
 * 为常用的 POSIX errno 提供更清晰的语义化名称，便于代码可读性
 *===========================================================================*/

/* 通用错误 - 映射到 POSIX errno */
#define OSAL_ERR_INVALID_POINTER        OSAL_EFAULT         /* 14: 无效指针 */
#define OSAL_ERR_NO_MEMORY              OSAL_ENOMEM         /* 12: 内存不足 */
#define OSAL_ERR_INVALID_SIZE           OSAL_EINVAL         /* 22: 无效参数/大小 */
#define OSAL_ERR_INVALID_ID             OSAL_EINVAL         /* 22: 无效ID */
#define OSAL_ERR_INVALID_PARAM          OSAL_EINVAL         /* 22: 无效参数 */
#define OSAL_ERR_NAME_TOO_LONG          OSAL_ENAMETOOLONG   /* 36: 名称过长 */
#define OSAL_ERR_NAME_TAKEN             OSAL_EEXIST         /* 17: 名称已存在 */
#define OSAL_ERR_NAME_NOT_FOUND         OSAL_ENOENT         /* 2: 名称未找到 */
#define OSAL_ERR_TIMEOUT                OSAL_ETIMEDOUT      /* 110: 超时 */
#define OSAL_ERR_NOT_IMPLEMENTED        OSAL_ENOSYS         /* 38: 未实现 */
#define OSAL_ERR_BUSY                   OSAL_EBUSY          /* 16: 资源忙 */
#define OSAL_ERR_PERMISSION             OSAL_EPERM          /* 1: 权限不足 */
#define OSAL_ERR_NOT_SUPPORTED          OSAL_EOPNOTSUPP     /* 95: 不支持的操作 */
#define OSAL_ERR_ALREADY_EXISTS         OSAL_EEXIST         /* 17: 已存在 */
#define OSAL_ERR_WOULD_BLOCK            OSAL_EWOULDBLOCK    /* 11: 操作会阻塞 */
#define OSAL_ERR_INTERRUPTED            OSAL_EINTR          /* 4: 操作被中断 */
#define OSAL_ERR_BAD_ADDRESS            OSAL_EFAULT         /* 14: 错误的地址 */
#define OSAL_ERR_RESOURCE_LIMIT         OSAL_EMFILE         /* 24: 资源限制 */
#define OSAL_ERR_GENERIC                OSAL_EIO            /* 5: 通用I/O错误 */

/* OSAL 特定错误码 - 使用未被 errno 占用的值 (200+) */
#define OSAL_ERR_ADDRESS_MISALIGNED     0xC8             /* 地址未对齐 */
#define OSAL_ERR_INVALID_INT_NUM        0xC9             /* 无效中断号 */
#define OSAL_ERR_INVALID_PRIORITY       0xCA             /* 无效优先级 */
#define OSAL_ERR_INVALID_STATE          0xCB             /* 无效状态 */
#define OSAL_ERR_NO_FREE_IDS            0xCC             /* 无可用ID */

/* 信号量相关错误码 */
#define OSAL_ERR_SEM_FAILURE            0xD2             /* 信号量失败 */
#define OSAL_ERR_SEM_TIMEOUT            OSAL_ETIMEDOUT  /* 110: 信号量超时 */
#define OSAL_ERR_SEM_NOT_FULL           0xD3             /* 信号量未满 */
#define OSAL_ERR_INVALID_SEM_VALUE      0xD4             /* 无效信号量值 */

/* 队列相关错误码 */
#define OSAL_ERR_QUEUE_EMPTY            0xDC             /* 队列为空 */
#define OSAL_ERR_QUEUE_FULL             0xDD             /* 队列已满 */
#define OSAL_ERR_QUEUE_TIMEOUT          OSAL_ETIMEDOUT  /* 110: 队列超时 */
#define OSAL_ERR_QUEUE_INVALID_SIZE     OSAL_EINVAL     /* 22: 队列大小无效 */
#define OSAL_ERR_QUEUE_ID               0xDE             /* 队列ID错误 */

/* 文件相关错误码 */
#define OSAL_ERR_FILE                   OSAL_EIO        /* 5: 文件错误 */

/* 定时器相关错误码 */
#define OSAL_ERR_TIMER_INVALID_ARGS     0xE6             /* 定时器参数无效 */
#define OSAL_ERR_TIMER_ID               0xE7             /* 定时器ID错误 */
#define OSAL_ERR_TIMER_UNAVAILABLE      OSAL_EAGAIN     /* 11: 定时器不可用 */
#define OSAL_ERR_TIMER_INTERNAL         0xE8             /* 定时器内部错误 */

/*===========================================================================
 * errno访问函数
 *===========================================================================*/

/**
 * @brief 获取当前errno值
 * @return errno值
 */
int32_t OSAL_get_errno(void);

/**
 * @brief 设置errno值
 * @param err 错误码
 */
void OSAL_set_errno(int32_t err);

/**
 * @brief 获取错误码对应的错误描述字符串
 * @param errnum 错误码
 * @return 错误描述字符串
 */
const char *OSAL_strerror(int32_t errnum);

/*===========================================================================
 * OSAL状态码转字符串
 *===========================================================================*/

/**
 * @brief 获取OSAL状态码对应的名称字符串
 * @param status_code OSAL状态码（OSAL_SUCCESS、OSAL_ERR_*等）
 * @return 状态码名称字符串
 */
const char *OSAL_get_status_name(int32_t status_code);

/**
 * @brief 获取OSAL状态码对应的描述字符串
 * @param status OSAL状态码
 * @return 状态码描述字符串
 */
const char *OSAL_status_to_string(osal_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_ERRNO_H */
