# ZDT_X42S 步进电机驱动（STM32F103 + HAL + CubeMX）

> 创建日期：2026-11-17  
> 当前进度：第 4～8 步代码已完成并通过编译，等待可用电机进行硬件复核

## 1. 工程目标

使用 STM32、HAL 库和 TTL 串口，逐步编写 ZDT42 闭环步进电机驱动。

当前实际使用的驱动板丝印型号为 **X42_V1.3**，属于老板卡。工程名称虽为 `ZDT_X42S`，但协议和返回格式应以实际电机的响应为准，不能完全照搬新版 X42S 手册。

本 README 记录到第 8 步。第 4～8 步已经完成代码编写和 Keil 全量编译；由于当前电机损坏，驱动版读取版本、运动控制和反馈读取的实际硬件效果仍待复核。

## 2. 开发环境

- 主控：STM32F103C8T6（以 CubeMX 工程实际选择的型号为准）
- IDE：Keil 5 / MDK-ARM
- 工程配置：STM32CubeMX
- 固件库：STM32 HAL 库
- 通信方式：UART TTL
- 调试器：ST-Link

## 3. 学习进度

| 步骤 | 内容 | 状态 |
|---|---|---|
| 第 1 步 | 硬件、接线与 TTL 通信验证 | 已完成 |
| 第 2 步 | 理解通信协议和已验证命令 | 已完成 |
| 第 3 步 | CubeMX 配置及 STM32 串口收发验证 | 已完成 |
| 第 4 步 | 驱动骨架与读取版本接口 | 代码完成，待硬件复核 |
| 第 5 步 | 使能、失能与立即停止接口 | 代码完成，待硬件验证 |
| 第 6 步 | 速度模式控制接口 | 代码完成，待硬件验证 |
| 第 7 步 | 相对与绝对位置控制接口 | 代码完成，待硬件验证 |
| 第 8 步 | 转速、位置反馈与异常处理 | 代码完成，待硬件验证 |

## 4. 已确认的电机参数

| 项目 | 参数 |
|---|---|
| 电机地址 | `0x05`，菜单 `ID_Addr = 5` |
| 波特率 | `115200` |
| 串口格式 | 8 位数据位、无校验、1 位停止位（8N1） |
| 通信端口功能 | `P_Serial = UART_FUN` |
| 校验方式 | 固定字节 `0x6B` |
| 控制命令应答 | `Response = Receive` |

## 5. 第 1 步：USB 转 TTL 验证结果

使用 USB 转 TTL 和串口助手完成了硬件验证。电机实际完成了低速正转和反转，停止、失能也正常。

### 5.1 接线原则

- USB 转 TTL 的 `TXD` 接电机 `RX`；
- USB 转 TTL 的 `RXD` 接电机 `TX`；
- 双方必须共地；
- 电机使用独立电源，不能由 USB 转 TTL 模块供电；
- 所有改线操作均在断电状态下进行；
- 不使用 RS232 电平直接连接 STM32 或 TTL 接口。

### 5.2 已验证命令

以下命令均以地址 `0x05`、固定校验 `0x6B` 为例。

| 功能 | 主机发送 | 电机正常返回 |
|---|---|---|
| 读取版本 | `05 1F 6B` | `05 1F 02 82 6B` |
| 使能 | `05 F3 AB 01 00 6B` | `05 F3 02 6B` |
| Emm 格式 10 RPM 正转 | `05 F6 00 00 0A 0A 00 6B` | `05 F6 02 6B` |
| Emm 格式 10 RPM 反转 | `05 F6 01 00 0A 0A 00 6B` | `05 F6 02 6B` |
| 立即停止 | `05 FE 98 00 6B` | `05 FE 02 6B` |
| 失能 | `05 F3 AB 00 00 6B` | `05 F3 02 6B` |

注意：当前电机接受的是 **Emm 风格的 8 字节速度命令**。曾测试 9 字节 X 固件速度格式，电机返回 `05 00 EE 6B`，其中 `EE` 表示命令格式错误，因此当前不使用该格式。

## 6. 第 2 步：协议要点

### 6.1 基本帧结构

```text
地址 + 功能码 + 数据区 + 固定校验字节
```

不同命令的数据区长度可以不同，不能假设所有命令都具有相同长度。

### 6.2 Emm 速度命令格式

以 10 RPM 正转为例：

```text
05 F6 00 00 0A 0A 00 6B
```

| 字段 | 字节 | 含义 |
|---|---|---|
| 地址 | `05` | 地址为 5 的电机 |
| 功能码 | `F6` | 速度模式控制 |
| 方向 | `00` | `00` 为 CW，`01` 为 CCW |
| 速度 | `00 0A` | `0x000A = 10 RPM`，高字节在前 |
| 加速度 | `0A` | 加速度档位 10 |
| 同步标志 | `00` | 立即执行 |
| 固定校验 | `6B` | 当前模式固定填写 |

速度字段为 16 位，多字节数据按高字节在前发送。例如：

```text
20 RPM = 0x0014 → 00 14
30 RPM = 0x001E → 00 1E
```

### 6.3 同步标志

- `00`：收到命令后立即执行；
- `01`：先缓存命令，等待多机同步运动触发命令后执行。

当前单电机基础测试统一使用 `00`。

### 6.4 返回状态

- `02`：命令接收正确；
- `E2`：参数错误、条件不满足或存在保护状态；
- `EE`：命令格式错误。

`02` 只表示命令被正确接收，不等于位置运动已经完成。读取类命令的返回内容需要结合对应功能码解析，不能把所有返回中的 `02` 都解释为通用成功状态。

## 7. 第 3 步：STM32 USART1 最小通信测试

### 7.1 接线

断电后，将原来连接 USB 转 TTL 的电机通信线改接到 STM32：

| STM32 | 电机驱动板 |
|---|---|
| `PA9 / USART1_TX` | 电机 `RX` |
| `PA10 / USART1_RX` | 电机 `TX` |
| `GND` | 电机 `GND` |

电机继续使用独立电源。不要让 USB 转 TTL 和 STM32 同时驱动电机通信线。

### 7.2 CubeMX 配置

1. `System Core → SYS → Debug` 选择 `Serial Wire`；
2. `Connectivity → USART1`，模式选择 `Asynchronous`；
3. 参数设置为 `115200`、8 Bits、None、1 Stop Bit、TX/RX；
4. 确认 `PA9 = USART1_TX`、`PA10 = USART1_RX`；
5. 本阶段使用阻塞收发，暂不开启 USART1 中断和 DMA；
6. 时钟保持开发板原有可用配置；
7. `Project Manager → Toolchain/IDE` 选择 `MDK-ARM`，生成 Keil 工程。

### 7.3 最小测试代码

本测试只发送读取版本命令，不让电机运动。

在 `main.c` 的 `USER CODE BEGIN PV` 区域添加：

```c
/* USER CODE BEGIN PV */
uint8_t tx_frame[3] = {0x05, 0x1F, 0x6B};
uint8_t rx_frame[5] = {0};

HAL_StatusTypeDef tx_result = HAL_OK;
HAL_StatusTypeDef rx_result = HAL_OK;
volatile uint8_t test_done = 0;
/* USER CODE END PV */
```

在外设初始化完成后的 `USER CODE BEGIN 2` 区域添加：

```c
/* USER CODE BEGIN 2 */
HAL_Delay(1000);

tx_result = HAL_UART_Transmit(
    &huart1, tx_frame, sizeof(tx_frame), 100);

rx_result = HAL_UART_Receive(
    &huart1, rx_frame, sizeof(rx_frame), 500);

test_done = 1;
/* USER CODE END 2 */
```

程序上电后等待 1 秒，发送 `05 1F 6B`，然后最多等待 500 ms 接收 5 字节。测试只执行一次，不在 `while (1)` 中循环发送。

### 7.4 Keil 调试观察方法

1. 编译并下载程序；
2. 进入 Keil Debug；
3. 在 `test_done = 1;` 处设置断点；
4. 全速运行，命中断点后在 Watch 窗口观察 `tx_result`、`rx_result` 和 `rx_frame`。

预期结果：

```text
tx_result = 0
rx_result = 0
```

`0` 表示 `HAL_OK`，`3` 表示 `HAL_TIMEOUT`。

根据当前电机在串口助手下的实际响应，预期接收缓冲区为：

```text
rx_frame[0] = 0x05
rx_frame[1] = 0x1F
rx_frame[2] = 0x02
rx_frame[3] = 0x82
rx_frame[4] = 0x6B
```

如果出现超时，优先检查 TX/RX 是否交叉、是否共地、波特率是否一致，以及 USB 转 TTL 是否仍连接在电机通信线上。

### 7.5 实际验证结果

2026-11-17 已在 Keil Watch 窗口确认：

```text
tx_result = 0x00
rx_result = 0x00
rx_frame  = 05 1F 02 82 6B
```

这表明 STM32 已通过 USART1 独立完成命令发送和电机回复接收，双向 TTL 通信正常，第 3 步完成。Watch 中 `0x82 '?'`、`0x6B 'k'` 只是同时显示字节对应的 ASCII 字符，不是通信错误。

## 8. 第 4 步：驱动文件与接口骨架

### 8.1 驱动文件

```text
Project/
└─ Core/
   ├─ Inc/
   │  └─ zdt42.h
   └─ Src/
      ├─ main.c
      └─ zdt42.c
```

`zdt42.c` 已加入 Keil 工程的 `APP/Src` 分组，`zdt42.h` 已加入 `APP/Inc` 分组。`Core/Inc` 已在 C/C++ Include Path 中。

### 8.2 分层职责

```text
main.c 应用层
    ↓ 调用驱动接口
zdt42.c / zdt42.h 驱动层
    ↓ 调用 HAL UART
USART1 / PA9 / PA10
    ↓ TTL
ZDT42 电机
```

- `main.c` 不再直接拼接十六进制协议帧；
- `zdt42.c` 负责组帧、UART 收发、回复检查和错误转换；
- `ZDT42_Send()`、`ZDT42_Receive()` 使用 `static`，仅供驱动内部调用；
- UART 句柄通过 `ZDT42_Init(&huart1)` 传入，驱动内部不直接依赖全局 `huart1`。

### 8.3 当前公开接口

```c
ZDT42_Status ZDT42_Init(UART_HandleTypeDef *huart);
ZDT42_Status ZDT42_ReadVersion(uint8_t id, uint8_t *rx, uint16_t rx_size);
```

`ZDT42_Status` 是自定义枚举返回值，用于告诉调用者函数是否成功以及失败原因。

| 枚举值 | 数值 | 含义 |
|---|---:|---|
| `ZDT42_OK` | 0 | 执行成功 |
| `ZDT42_PARAM_ERROR` | 1 | 本地参数错误 |
| `ZDT42_HAL_ERROR` | 2 | HAL 收发失败 |
| `ZDT42_TIMEOUT` | 3 | 收发超时 |
| `ZDT42_RESPONSE_ERROR` | 4 | 回复地址、功能码或帧尾不符合要求 |
| `ZDT42_MOTOR_ERROR` | 5 | 电机返回 `E2`，参数或运行条件不满足 |
| `ZDT42_FORMAT_ERROR` | 6 | 电机返回 `EE`，命令格式错误 |

函数返回值表示执行状态，`rx` 指向的数组用于保存电机返回的数据，两者职责不同。

### 8.4 当前 main.c 调用方式

```c
zdt_status = ZDT42_Init(&huart1);

if (zdt_status == ZDT42_OK)
{
    HAL_Delay(1000);
    zdt_status = ZDT42_ReadVersion(
        0x05, zdt_version, sizeof(zdt_version));
}

test_done = 1;
```

`ZDT42_ReadVersion()` 当前固定接收 5 字节，这是根据 X42_V1.3 老板卡实测回复 `05 1F 02 82 6B` 编写的，不能视为所有 ZDT42 固件的通用返回长度。

## 9. 第 5 步：使能、失能与立即停止

### 9.1 新增接口

```c
ZDT42_Status ZDT42_Enable(uint8_t id, uint8_t enable);
ZDT42_Status ZDT42_Stop(uint8_t id);
```

调用示例：

```c
zdt_status = ZDT42_Enable(0x05, 1);  /* 使能 */
zdt_status = ZDT42_Stop(0x05);       /* 立即停止 */
zdt_status = ZDT42_Enable(0x05, 0);  /* 失能 */
```

`ZDT42_Enable()` 只允许 `enable` 为 `0` 或 `1`，其他值返回 `ZDT42_PARAM_ERROR`。当前接口针对单电机控制，`id = 0` 不作为这些接口的有效参数。

### 9.2 驱动生成的命令

| 接口 | 生成的命令 |
|---|---|
| `ZDT42_Enable(0x05, 1)` | `05 F3 AB 01 00 6B` |
| `ZDT42_Enable(0x05, 0)` | `05 F3 AB 00 00 6B` |
| `ZDT42_Stop(0x05)` | `05 FE 98 00 6B` |

同步标志当前固定为 `00`，表示立即执行。

### 9.3 控制命令回复检查

驱动内部新增静态函数 `ZDT42_ControlCommand()`，统一完成：

1. 发送控制命令；
2. 接收 4 字节回复；
3. 检查回复地址和 `0x6B` 帧尾；
4. 识别电机返回的 `02`、`E2`、`EE`；
5. 检查成功回复中的功能码是否与原命令一致。

| 回复情况 | 驱动返回 |
|---|---|
| `02` 且地址、功能码、帧尾正确 | `ZDT42_OK` |
| 电机返回 `E2` | `ZDT42_MOTOR_ERROR` |
| 电机返回 `EE` | `ZDT42_FORMAT_ERROR` |
| 地址、功能码或帧尾异常 | `ZDT42_RESPONSE_ERROR` |
| 规定时间内未收到完整回复 | `ZDT42_TIMEOUT` |

## 10. 第 6 步：速度模式控制

### 10.1 接口与调用

```c
ZDT42_Status ZDT42_SetSpeed(uint8_t id, int16_t rpm, uint8_t accel);
```

```c
ZDT42_SetSpeed(ZDT42_MOTOR_ID, 10, 10);   /* 正转 10 RPM */
ZDT42_SetSpeed(ZDT42_MOTOR_ID, -10, 10);  /* 反转 10 RPM */
ZDT42_SetSpeed(ZDT42_MOTOR_ID, 30, 10);   /* 正转 30 RPM */
```

- `rpm > 0`：正转；`rpm < 0`：反转；
- 有效范围为 ±1～±3000 RPM；
- `rpm = 0` 返回 `ZDT42_PARAM_ERROR`，停止应调用 `ZDT42_Stop()`；
- `accel` 为 0～255，0 表示直接启动，数值越大加速越快；
- 同步标志固定为 `00`，收到命令后立即执行。

### 10.2 组帧与速度拆分

驱动根据 `rpm` 的正负设置方向，对速度绝对值进行高、低字节拆分：

```c
tx[3] = (uint8_t)(speed >> 8);
tx[4] = (uint8_t)(speed & 0xFFU);
```

| 函数调用 | 生成的命令 |
|---|---|
| `ZDT42_SetSpeed(0x05, 10, 10)` | `05 F6 00 00 0A 0A 00 6B` |
| `ZDT42_SetSpeed(0x05, -10, 10)` | `05 F6 01 00 0A 0A 00 6B` |
| `ZDT42_SetSpeed(0x05, 30, 10)` | `05 F6 00 00 1E 0A 00 6B` |

发送后复用 `ZDT42_ControlCommand()` 检查电机回复。

## 11. 第 7 步：相对与绝对位置控制

### 11.1 接口

```c
ZDT42_Status ZDT42_MoveRelative(
    uint8_t id, int32_t angle_deg, uint16_t rpm, uint8_t accel);

ZDT42_Status ZDT42_MoveAbsolute(
    uint8_t id, int32_t angle_deg, uint16_t rpm, uint8_t accel);
```

```c
ZDT42_MoveRelative(ZDT42_MOTOR_ID, 90, 10, 10);
ZDT42_MoveRelative(ZDT42_MOTOR_ID, -90, 10, 10);
ZDT42_MoveAbsolute(ZDT42_MOTOR_ID, 90, 10, 10);
```

位置命令中的方向由 `angle_deg` 的正负决定，所以 `rpm` 只填写 1～3000 的正数。当前角度接口使用整数度。

### 11.2 角度与脉冲换算

当前默认条件：

- 电机步距角 1.8°；
- `MStep = 16`；
- 每圈脉冲数为 `200 × 16 = 3200`。

```text
脉冲数 ≈ |角度| × 3200 ÷ 360
90° → 800 个脉冲 → 0x00000320
```

代码使用 64 位整数计算、四舍五入，并检查结果是否超过协议的 32 位位置范围。实机使用前必须确认步距角和 `MStep`，否则目标角度会产生比例错误。

### 11.3 命令与模式

| 函数调用 | 生成的命令 |
|---|---|
| 相对 `+90°` | `05 FD 00 00 0A 0A 00 00 03 20 00 00 6B` |
| 相对 `-90°` | `05 FD 01 00 0A 0A 00 00 03 20 00 00 6B` |
| 绝对 `+90°` | `05 FD 00 00 0A 0A 00 00 03 20 01 00 6B` |

为兼顾 X42_V1.3 / Emm42 V5.0：

- 相对模式使用 `00`，相对上一输入目标位置；
- 绝对模式使用 `01`，相对坐标零点；
- 未使用新版手册增加的 `02` 实时位置相对模式。

正常连续执行多次相对 `+90°` 时应能累计运动，但该行为仍需实机确认。位置命令返回 `02` 只表示命令已接收，不代表运动到位。

## 12. 第 8 步：反馈读取与异常处理

### 12.1 新增接口

```c
ZDT42_Status ZDT42_ReadSpeed(uint8_t id, int16_t *rpm);
ZDT42_Status ZDT42_ReadPosition(uint8_t id, float *angle_deg);
```

```c
int16_t actual_rpm = 0;
float actual_angle = 0.0f;

zdt_status = ZDT42_ReadSpeed(ZDT42_MOTOR_ID, &actual_rpm);
zdt_status = ZDT42_ReadPosition(ZDT42_MOTOR_ID, &actual_angle);
```

两个接口都是输出参数与函数返回值分离：返回值表示通信和解析是否成功，指针指向的变量保存实际反馈。失败时不修改输出值。

### 12.2 转速解析

Emm 实时转速回复为：

```text
地址 + 35 + 符号 + 转速高字节 + 转速低字节 + 6B
```

- 符号 `00` 为正，`01` 为负；
- 16 位转速通过移位和按位或组合，不依赖指针强制类型转换；
- 单位为 RPM。

```text
05 35 00 00 0A 6B → +10 RPM
05 35 01 00 0A 6B → -10 RPM
```

### 12.3 位置解析

Emm 实时位置回复为：

```text
地址 + 36 + 符号 + 4 字节位置 + 6B
```

位置原始值按高字节在前组合。Emm 固件每 65536 个计数对应一圈：

```text
角度 = 原始位置 × 360 ÷ 65536
```

例如原始值 `0x00004000 = 16384`：

```text
16384 × 360 ÷ 65536 = 90°
```

符号字节为 `01` 时结果为负角度。注意：位置**命令**默认使用 3200 脉冲/圈，而位置**反馈**使用 65536 计数/圈，两者比例不同，不能混用。

### 12.4 统一查询和错误处理

读取类命令统一使用内部 `ZDT42_Query()`：

1. 发送 `地址 + 功能码 + 6B`；
2. 先接收 4 字节，及时识别 `E2/EE` 短错误帧；
3. 正常时继续接收剩余字节；
4. 检查地址、功能码和末尾 `6B`；
5. 检查符号字节是否合法；
6. 任何接收阶段超时都会返回，不会无限等待。

`ZDT42_ReadVersion()` 也已改为复用该查询函数。当前控制命令仍按 `Response = Receive` 的确认回复处理；位置到位 `9F`、自动等待到位和接收状态机尚未实现。

## 13. 编译与硬件验证状态

| 日期 | 范围 | Keil 全量编译结果 |
|---|---|---|
| 2026-11-18 | 第 4、5 步 | 0 Error、0 Warning |
| 2026-11-19 | 第 6 步 | 0 Error、0 Warning |
| 2026-11-20 | 第 7 步 | 0 Error、0 Warning |
| 2026-11-21 | 第 8 步 | 0 Error、0 Warning |

当前生成文件：

```text
Project/MDK-ARM/Project/Project.axf
Project/MDK-ARM/Project/Project.hex
```

编译通过只证明代码可以通过编译和链接。以下项目仍等待可用电机验证：

- 驱动版版本读取及使能、停止、失能；
- 至少 3 组速度及正反转；
- 相对 `+90°`、`-90°` 和连续多次相对运动；
- 绝对位置运动及坐标零点行为；
- 实际 `MStep` 与 3200 脉冲/圈假设是否一致；
- 转速和位置反馈的正负号、比例及数值合理性；
- 断线、无回复、`E2/EE` 和错误帧处理。

在硬件复核完成前，不将这些接口标记为“实机验证通过”。

## 14. 工程维护注意事项

- 在 CubeMX 重新生成代码时，自行添加的内容应放在 `USER CODE` 区域，避免被覆盖；
- 当前驱动使用阻塞式 HAL UART 收发，后续再进行接收方式和超时机制优化；
- 在确认接收超时或数据异常时，不要连续发送运动命令；
- 电机运动测试应保持空载、固定电机，并确保可以及时断电；
- 不要带电插拔电机线、供电线或通信线。
