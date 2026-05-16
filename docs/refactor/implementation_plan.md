# 实施计划

## 14. 启动流程

### 14.1 系统启动序列

1. Bootloader启动
 ├── 检查启动标志
    ├── 选择启动分区（A或B）
    └── 加载Linux内核

2. Linux内核启动
 ├── 初始化硬件
    ├── 挂载根文件系统
    └── 启动init进程

3. Init进程启动PMC
 ├── 加载配置文件（/etc/pmc/pmc.conf）
    ├── 创建共享内存（/dev/shm/pmc_shm）
    ├── 启动Supervisor进程
    └── Supervisor启动子进程

4. Supervisor启动子进程
 ├── 启动Telecommand进程（最高优先级）
    ├── 启动Telemetry进程
    ├── 启动Logger进程
    ├── 启动Firmware进程
    └── 开始心跳监控

5. 各进程初始化
 ├── Telecommand进程：
    │   ├── 加载ACL配置
    │   ├── 初始化PDL服务（Satellite/BMC/MCU）
    │   ├── 设置实时调度（SCHED_FIFO, priority=99）
    │   ├── 绑定CPU0
    │   ├── 锁定内存
    │   └── 启动CAN接收线程
    │
    ├── Telemetry进程：
    │   ├── 加载ACL配置
    │   ├── 初始化PDL服务
    │   ├── 映射共享内存（遥测缓冲区）
    │   └── 启动采集线程
    │
    ├── Logger进程：
    │   ├── 映射共享内存（日志环形缓冲区）
    │   ├── 创建日志目录
    │   └── 启动日志收集线程
    │
    └── Firmware进程：
	 ├── 检查启动分区
	 ├── 标记启动成功
	 └── 进入空闲状态

6. 系统就绪
 ├── 向卫星平台发送心跳
    ├── 开始接收遥控遥测命令
    └── 后台采集遥测数据

### 14.2 启动脚本

``` shell
#!/bin/bash

# /etc/init.d/pmc

# PMC启动脚本

PMC_CONF="/etc/pmc/pmc.conf"
PMC_SUPERVISOR="/usr/bin/pmc_supervisor"
PMC_PID_FILE="/var/run/pmc_supervisor.pid"
PMC_LOG_DIR="/var/log/pmc"

start() {
  echo "Starting PMC."

  # 1. 检查配置文件

  if [ ! -f "$PMC_CONF" ]; then
	  echo "Error: Config file not found: $PMC_CONF"
	  exit 1
  fi

  # 2. 创建日志目录

  mkdir -p "$PMC_LOG_DIR"
  mkdir -p "$PMC_LOG_DIR/archive"
  mkdir -p "$PMC_LOG_DIR/crash"

  # 3. 清理旧的共享内存

  rm -f /dev/shm/pmc_*

  # 4. 设置CAN接口

  ip link set can0 type can bitrate 500000
  ip link set can0 up

  # 5. 启动Supervisor进程

  $PMC_SUPERVISOR --config $PMC_CONF --daemon

  # 6. 等待启动完成

  sleep 2

  # 7. 检查进程状态

  if [ -f "$PMC_PID_FILE" ]; then
	  PID=$(cat $PMC_PID_FILE)
	  if ps -p $PID > /dev/null; then
		  echo "PMC started successfully (PID: $PID)"
	  else
		  echo "Error: PMC failed to start"
		  exit 1
	  fi
  else
	  echo "Error: PID file not found"
	  exit 1
  fi
}

stop() {
  echo "Stopping PMC."

  if [ -f "$PMC_PID_FILE" ]; then
	  PID=$(cat $PMC_PID_FILE)
	  kill -TERM $PID

	  # 等待进程退出

	  for i in {110}; do
		  if ! ps -p $PID > /dev/null; then
			  echo "PMC stopped"
			  rm -f $PMC_PID_FILE
			  return 0
		  fi
		  sleep 1
	  done

	  # 强制杀死

	  kill -KILL $PID
	  rm -f $PMC_PID_FILE
	  echo "PMC force stopped"
  else
	  echo "PMC is not running"
  fi
}

status() {
  if [ -f "$PMC_PID_FILE" ]; then
	  PID=$(cat $PMC_PID_FILE)
	  if ps -p $PID > /dev/null; then
		  echo "PMC is running (PID: $PID)"

		  # 显示子进程状态

		  echo ""
		  echo "Child processes:"
		  ps -eo pid,comm,stat,pri,nice | grep -E "pmc_|PID"
	  else
		  echo "PMC is not running (stale PID file)"
	  fi
  else
	  echo "PMC is not running"
  fi
}

case "$1" in
  start)
	  start
	  ;;
  stop)
	  stop
	  ;;
  restart)
	  stop
	  start
	  ;;
  status)
	  status
	  ;;
  *)
	  echo "Usage: $0 {start|stop|restart|status}"
	  exit 1
	  ;;
esac

exit 0
```

---
