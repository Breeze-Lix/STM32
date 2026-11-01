# STM32 + JY60 姿态传感器驱动

基于 STM32F103 + HAL 库，用 DMA + 空闲中断接收 JY60 数据，
解析出 Roll / Pitch / Yaw，通过串口输出供上位机可视化。

## 1. 硬件

| 项目 | 内容 |
|---|---|
| 主控 | STM32F103C8T6（64KB Flash / 20KB RAM） |
| 传感器 | 维特智能 JY60 六轴姿态模块 |
| 调试工具 | USB-TTL（CH340） |

### 接线表

| 信号 | MCU 引脚 | 接到哪里 |
|---|---|---|
| 传感器 TX | PA10（USART1_RX） | JY60 的 TX |
| 调试 TX | PA2（USART2_TX） | USB-TTL 的 RX |
| 电源 | 3.3V / GND | 共地 |

> 传感器只接 TX、不接 RX —— 单向接收，避免调试输出被传感器误收。

## 2. 软件环境

| 项目 | 版本 / 说明 |
|---|---|
| STM32CubeMX | 6.x |
| Keil MDK | 5.x（ARMCC） |
| 工程文件 | `MDK-ARM/Project.uvprojx` |

⚠️ **必须勾选 `Options for Target → Target → Use MicroLIB`**，
否则 `printf` 会走半主机（semihosting），脱机运行卡死无输出。

## 3. 模块划分

| 文件 | 职责 | 来源 |
|---|---|---|
| `Ring.c/h` | 通用环形缓冲区 | 自写，可移植 |
| `UARTRx.c/h` | DMA + 空闲中断接收 | 自写，可移植 |
| `Imu.c/h` | JY60 协议解析 | 自写 |
| `main.c` | 主循环、printf 重定向 | CubeMX + 自写 |
| 其余 | CubeMX 生成 | 只在 `USER CODE` 区内修改 |

## 4. 数据流

```
JY60 ──USART1/DMA──> UARTRx 回调 ──> Ring ──> IMU_Poll
                                                  │
                                    解析 0x52 / 0x53 帧
                                                  │
                                    Roll / Pitch / Yaw / Rate
                                                  │
                              printf(FireWater) ──> USART2 ──> PC
```

## 5. 通信参数

| 项目 | 值 |
|---|---|
| 传感器波特率 | 9600（JY60 固定，不可修改） |
| 调试串口 | 115200 |
| 输出速率 | 20Hz |
| 帧格式 | `0x55 + 类型 + 4×int16(小端) + 校验`，共 11 字节 |
| 校验方式 | 前 10 字节求和取低 8 位（**包含 0x55**） |
| 角度换算 | `raw × 180 / 32768` |
| 角速度换算 | `raw × 2000 / 32768` |

## 6. 编译与运行

1. 用 Keil 打开 `MDK-ARM/Project.uvprojx`
2. 确认已勾 Use MicroLIB
3. Rebuild → 下载
4. 串口助手 115200 打开，应看到三列数字 `roll,pitch,yaw`

## 7. 上位机显示（可选）

VOFA+：数据源选串口 → 115200 → 协议 **FireWater** →
添加 **cube** 控件 → 欧拉角模式 → 单位"角度" →
X=I0(Roll) / Y=I1(Pitch) / Z=I2(Yaw)

## 8. 已知问题

| 问题 | 原因 | 影响 |
|---|---|---|
| Yaw 缓慢漂移 | 六轴无磁力计，Yaw 由角速度积分得出 | 静止数分钟会偏几度；Roll/Pitch 不受影响 |
| 输出率固定 20Hz | 传感器出厂固定 | 不适合高速闭环控制 |
| 未做 Roll/Pitch 零点 | 靠重力解算，本身是绝对角 | 安装不平时显示非 0 |

## 9. 待办

- [ ] 5 分钟连续运行稳定性测试
- [ ] 断开传感器后自动恢复实测
- [ ] `UARTRx.c/h` 文件名统一为 `UartRx.c/h`