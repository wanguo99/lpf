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

#ifndef OSAL_ERRNO_API_H
#define OSAL_ERRNO_API_H

#include "osal_types_api.h"

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
#define OSAL_SUCCESS                    0

/*===========================================================================
 * POSIX errno 常量定义
 * 按照 Linux errno.h 的顺序定义，提供跨平台的统一错误码
 *===========================================================================*/

#define OSAL_EPERM           1   /* 操作不允许 */
#define OSAL_ENOENT          2   /* 文件或目录不存在 */
#define OSAL_ESRCH           3   /* 进程不存在 */
#define OSAL_EINTR           4   /* 系统调用被中断 */
#define OSAL_EIO             5   /* I/O错误 */
#define OSAL_ENXIO           6   /* 设备或地址不存在 */
#define OSAL_E2BIG           7   /* 参数列表过长 */
#define OSAL_ENOEXEC         8   /* 执行格式错误 */
#define OSAL_EBADF           9   /* 文件描述符错误 */
#define OSAL_ECHILD          10  /* 子进程不存在 */
#define OSAL_EAGAIN          11  /* 资源暂时不可用 */
#define OSAL_ENOMEM          12  /* 内存不足 */
#define OSAL_EACCES          13  /* 权限不足 */
#define OSAL_EFAULT          14  /* 地址错误 */
#define OSAL_ENOTBLK         15  /* 需要块设备 */
#define OSAL_EBUSY           16  /* 设备或资源忙 */
#define OSAL_EEXIST          17  /* 文件已存在 */
#define OSAL_EXDEV           18  /* 跨设备链接 */
#define OSAL_ENODEV          19  /* 设备不存在 */
#define OSAL_ENOTDIR         20  /* 不是目录 */
#define OSAL_EISDIR          21  /* 是目录 */
#define OSAL_EINVAL          22  /* 参数无效 */
#define OSAL_ENFILE          23  /* 系统打开文件过多 */
#define OSAL_EMFILE          24  /* 进程打开文件过多 */
#define OSAL_ENOTTY          25  /* 不是终端设备 */
#define OSAL_ETXTBSY         26  /* 文本文件忙 */
#define OSAL_EFBIG           27  /* 文件过大 */
#define OSAL_ENOSPC          28  /* 设备空间不足 */
#define OSAL_ESPIPE          29  /* 非法seek */
#define OSAL_EROFS           30  /* 只读文件系统 */
#define OSAL_EMLINK          31  /* 链接过多 */
#define OSAL_EPIPE           32  /* 管道破裂 */
#define OSAL_EDOM            33  /* 数学参数超出范围 */
#define OSAL_ERANGE          34  /* 数学结果无法表示 */
#define OSAL_EDEADLK         35  /* 资源死锁 */
#define OSAL_ENAMETOOLONG    36  /* 文件名过长 */
#define OSAL_ENOLCK          37  /* 锁不可用 */
#define OSAL_ENOSYS          38  /* 功能未实现 */
#define OSAL_ENOTEMPTY       39  /* 目录非空 */
#define OSAL_ELOOP           40  /* 符号链接层次过多 */
#define OSAL_EWOULDBLOCK     OSAL_EAGAIN  /* 操作会阻塞 */
#define OSAL_ENOMSG          42  /* 无消息 */
#define OSAL_EIDRM           43  /* 标识符已删除 */
#define OSAL_ECHRNG          44  /* 通道号超出范围 */
#define OSAL_EL2NSYNC        45  /* 2级不同步 */
#define OSAL_EL3HLT          46  /* 3级停止 */
#define OSAL_EL3RST          47  /* 3级复位 */
#define OSAL_ELNRNG          48  /* 链接号超出范围 */
#define OSAL_EUNATCH         49  /* 协议驱动未连接 */
#define OSAL_ENOCSI          50  /* 无CSI结构 */
#define OSAL_EL2HLT          51  /* 2级停止 */
#define OSAL_EBADE           52  /* 无效交换 */
#define OSAL_EBADR           53  /* 无效请求描述符 */
#define OSAL_EXFULL          54  /* 交换满 */
#define OSAL_ENOANO          55  /* 无阳极 */
#define OSAL_EBADRQC         56  /* 无效请求码 */
#define OSAL_EBADSLT         57  /* 无效槽 */
#define OSAL_EDEADLOCK       OSAL_EDEADLK
#define OSAL_EBFONT          59  /* 字体文件格式错误 */
#define OSAL_ENOSTR          60  /* 不是流设备 */
#define OSAL_ENODATA         61  /* 无数据 */
#define OSAL_ETIME           62  /* 定时器超时 */
#define OSAL_ENOSR           63  /* 流资源不足 */
#define OSAL_ENONET          64  /* 机器不在网络上 */
#define OSAL_ENOPKG          65  /* 包未安装 */
#define OSAL_EREMOTE         66  /* 对象是远程的 */
#define OSAL_ENOLINK         67  /* 链接已断开 */
#define OSAL_EADV            68  /* 通告错误 */
#define OSAL_ESRMNT          69  /* Srmount错误 */
#define OSAL_ECOMM           70  /* 发送时通信错误 */
#define OSAL_EPROTO          71  /* 协议错误 */
#define OSAL_EMULTIHOP       72  /* 多跳尝试 */
#define OSAL_EDOTDOT         73  /* RFS特定错误 */
#define OSAL_EBADMSG         74  /* 消息错误 */
#define OSAL_EOVERFLOW       75  /* 值对数据类型过大 */
#define OSAL_ENOTUNIQ        76  /* 名称在网络上不唯一 */
#define OSAL_EBADFD          77  /* 文件描述符状态错误 */
#define OSAL_EREMCHG         78  /* 远程地址改变 */
#define OSAL_ELIBACC         79  /* 无法访问共享库 */
#define OSAL_ELIBBAD         80  /* 共享库损坏 */
#define OSAL_ELIBSCN         81  /* .lib节损坏 */
#define OSAL_ELIBMAX         82  /* 链接共享库过多 */
#define OSAL_ELIBEXEC        83  /* 无法直接执行共享库 */
#define OSAL_EILSEQ          84  /* 非法字节序列 */
#define OSAL_ERESTART        85  /* 应重启系统调用 */
#define OSAL_ESTRPIPE        86  /* 流管道错误 */
#define OSAL_EUSERS          87  /* 用户过多 */
#define OSAL_ENOTSOCK        88  /* 非socket操作 */
#define OSAL_EDESTADDRREQ    89  /* 需要目标地址 */
#define OSAL_EMSGSIZE        90  /* 消息过长 */
#define OSAL_EPROTOTYPE      91  /* socket协议类型错误 */
#define OSAL_ENOPROTOOPT     92  /* 协议不可用 */
#define OSAL_EPROTONOSUPPORT 93  /* 协议不支持 */
#define OSAL_ESOCKTNOSUPPORT 94  /* socket类型不支持 */
#define OSAL_EOPNOTSUPP      95  /* 操作不支持 */
#define OSAL_EPFNOSUPPORT    96  /* 协议族不支持 */
#define OSAL_EAFNOSUPPORT    97  /* 地址族不支持 */
#define OSAL_EADDRINUSE      98  /* 地址已使用 */
#define OSAL_EADDRNOTAVAIL   99  /* 地址不可用 */
#define OSAL_ENETDOWN        100 /* 网络已关闭 */
#define OSAL_ENETUNREACH     101 /* 网络不可达 */
#define OSAL_ENETRESET       102 /* 网络连接复位 */
#define OSAL_ECONNABORTED    103 /* 连接中止 */
#define OSAL_ECONNRESET      104 /* 连接复位 */
#define OSAL_ENOBUFS         105 /* 缓冲区空间不足 */
#define OSAL_EISCONN         106 /* 已连接 */
#define OSAL_ENOTCONN        107 /* 未连接 */
#define OSAL_ESHUTDOWN       108 /* 传输端点已关闭 */
#define OSAL_ETOOMANYREFS    109 /* 引用过多 */
#define OSAL_ETIMEDOUT       110 /* 连接超时 */
#define OSAL_ECONNREFUSED    111 /* 连接被拒绝 */
#define OSAL_EHOSTDOWN       112 /* 主机已关闭 */
#define OSAL_EHOSTUNREACH    113 /* 主机不可达 */
#define OSAL_EALREADY        114 /* 操作已在进行 */
#define OSAL_EINPROGRESS     115 /* 操作正在进行 */
#define OSAL_ESTALE          116 /* 陈旧的文件句柄 */
#define OSAL_EUCLEAN         117 /* 结构需要清理 */
#define OSAL_ENOTNAM         118 /* 不是XENIX命名文件 */
#define OSAL_ENAVAIL         119 /* 无XENIX信号量 */
#define OSAL_EISNAM          120 /* 是命名文件 */
#define OSAL_EREMOTEIO       121 /* 远程I/O错误 */
#define OSAL_EDQUOT          122 /* 超出配额 */
#define OSAL_ENOMEDIUM       123 /* 无介质 */
#define OSAL_EMEDIUMTYPE     124 /* 介质类型错误 */
#define OSAL_ECANCELED       125 /* 操作已取消 */
#define OSAL_ENOKEY          126 /* 所需密钥不可用 */
#define OSAL_EKEYEXPIRED     127 /* 密钥已过期 */
#define OSAL_EKEYREVOKED     128 /* 密钥已撤销 */
#define OSAL_EKEYREJECTED    129 /* 密钥被服务拒绝 */
#define OSAL_EOWNERDEAD      130 /* 所有者死亡 */
#define OSAL_ENOTRECOVERABLE 131 /* 状态不可恢复 */
#define OSAL_ERFKILL         132 /* 因RF-kill而无法操作 */
#define OSAL_EHWPOISON       133 /* 内存页硬件错误 */

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
#define OSAL_ERR_ADDRESS_MISALIGNED     200             /* 地址未对齐 */
#define OSAL_ERR_INVALID_INT_NUM        201             /* 无效中断号 */
#define OSAL_ERR_INVALID_PRIORITY       202             /* 无效优先级 */
#define OSAL_ERR_INVALID_STATE          203             /* 无效状态 */
#define OSAL_ERR_NO_FREE_IDS            204             /* 无可用ID */

/* 信号量相关错误码 */
#define OSAL_ERR_SEM_FAILURE            210             /* 信号量失败 */
#define OSAL_ERR_SEM_TIMEOUT            OSAL_ETIMEDOUT  /* 110: 信号量超时 */
#define OSAL_ERR_SEM_NOT_FULL           211             /* 信号量未满 */
#define OSAL_ERR_INVALID_SEM_VALUE      212             /* 无效信号量值 */

/* 队列相关错误码 */
#define OSAL_ERR_QUEUE_EMPTY            220             /* 队列为空 */
#define OSAL_ERR_QUEUE_FULL             221             /* 队列已满 */
#define OSAL_ERR_QUEUE_TIMEOUT          OSAL_ETIMEDOUT  /* 110: 队列超时 */
#define OSAL_ERR_QUEUE_INVALID_SIZE     OSAL_EINVAL     /* 22: 队列大小无效 */
#define OSAL_ERR_QUEUE_ID               222             /* 队列ID错误 */

/* 文件相关错误码 */
#define OSAL_ERR_FILE                   OSAL_EIO        /* 5: 文件错误 */

/* 定时器相关错误码 */
#define OSAL_ERR_TIMER_INVALID_ARGS     230             /* 定时器参数无效 */
#define OSAL_ERR_TIMER_ID               231             /* 定时器ID错误 */
#define OSAL_ERR_TIMER_UNAVAILABLE      OSAL_EAGAIN     /* 11: 定时器不可用 */
#define OSAL_ERR_TIMER_INTERNAL         232             /* 定时器内部错误 */

/*===========================================================================
 * errno访问函数
 *===========================================================================*/

/**
 * @brief 获取当前errno值
 * @return errno值
 */
int32_t OSAL_GetErrno(void);

/**
 * @brief 设置errno值
 * @param err 错误码
 */
void OSAL_SetErrno(int32_t err);

/**
 * @brief 获取错误码对应的错误描述字符串
 * @param errnum 错误码
 * @return 错误描述字符串
 */
const char *OSAL_StrError(int32_t errnum);

/*===========================================================================
 * OSAL状态码转字符串
 *===========================================================================*/

/**
 * @brief 获取OSAL状态码对应的名称字符串
 * @param status_code OSAL状态码（OSAL_SUCCESS、OSAL_ERR_*等）
 * @return 状态码名称字符串
 */
const char *OSAL_GetStatusName(int32_t status_code);

/**
 * @brief 获取OSAL状态码对应的描述字符串
 * @param status OSAL状态码
 * @return 状态码描述字符串
 */
const char *OSAL_StatusToString(osal_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_ERRNO_API_H */
