# FreeRTOS_and_PID

> 本工程关于学习 FreeRTOS 的内容，同时进行 PID 的学习

## FreeRTOS 概念

### 是什么

FreeRTOS 是跑在单片机上的**调度器**，负责决定"此刻该哪个任务占用 CPU"。

单核 CPU **不可能真正并行**，它做的是快速切换：一个任务跑到阻塞点（`osDelay`、等队列）就换下一个，切得够快，看起来像同时。

### 三个核心概念

| 概念 | 含义 |
|---|---|
| **Task（任务）** | 一个独立的 `for(;;)` 函数，有自己的栈和优先级 |
| **Tick（节拍）** | 系统心跳，默认 1ms 一次；`osDelay(100)` 就是等 100 个 tick |
| **Priority（优先级）** | 数字越大越优先；高优先级不阻塞时，低优先级永远轮不到 |

### 和裸机 `while(1)` 的区别

| | 裸机 | FreeRTOS |
|---|---|---|
| 代码组织 | 所有事挤在一个循环 | 每件事一个任务，各写各的 |
| 某处卡住 | **全部停摆** | 只影响自己 |
| 加新功能 | 改整个循环 | 新建一个任务 |

### 两个必须记住的细节

**① 启用 RTOS 后 `main()` 的 `while(1)` 变成死代码**

`osKernelStart()` 不返回，CPU 交给调度器之后再也回不来。所有业务都要写进任务函数。

**② `osDelay` 和 `HAL_Delay` 完全不同**

| | 行为 |
|---|---|
| `HAL_Delay(100)` | CPU 死等 100ms，什么都干不了 |
| `osDelay(100)` | 主动交出 CPU，调度器去跑别的任务 |

### 三个踩过的坑

| 坑 | 现象 | 原因 |
|---|---|---|
| Timebase 没改成 TIM3 | 卡在 `printf`、LED 不闪 | FreeRTOS 和 HAL 抢 SysTick |
| 堆太小（3072） | 第二个任务创建不出来，且不报错 | `osThreadCreate` 返回 NULL |
| 栈太小（128） | 跑几秒 HardFault | `printf` 一个人就要 1KB |

### 任务间怎么传值

| 方式 | 语义 | 适合 |
|---|---|---|
| `volatile` 全局变量 | 最新值覆盖 | 目标速度、目标角度（旧值没意义） |
| 队列 | 每条都排队、不丢 | 指令、数据包（每条都要处理） |

判断依据：**旧值还有用吗？** 没用就用全局变量，有用就用队列。

## 接口一览

| 接口 | 地点 |
|---|---|
| PA0 / PA1 | 电机 A/B 相（编码器） |
| PA2 / PA3 | USART2_TX / RX |
| PB1 / PB0 | AIN1 / AIN2（TB6612） |
| PB6 | PWMA |

## 1. 新建 FreeRTOS 工程

**CubeMX 配置：**

1. 在 `Middleware and Software Packs` 里面找 FreeRTOS
2. 设置 `Interface` → **CMSIS_V1**
3. 设置 `Stack Size` → **256**
4. 在 `System Core` 里面找 SYS，设置 `Timebase Source` → **TIM3**
5. 生成代码，用板载灯闪灯检验，位置 `void StartDefaultTask` → `for` 循环

> **注**：记得开启 USART2（**不用配 DMA / NVIC**）并加入 `fputc` 重定向 + 勾选 **Use MicroLIB**，否则 `printf` 会卡死。

## 2. 任务并行

**① 加第二个任务**

`FreeRTOS → Tasks and Queues → Tasks → Add`：

| 参数 | 填写 |
|---|---|
| Task Name | `sinTask` |
| Priority | `osPriorityNormal` |
| Stack Size | `256` |
| Entry Function | `StartSinTask` |

**② 两个任务用不同周期，先验证调度**

`defaultTask` 打印 `A`、延时 100ms；`sinTask` 打印 `B`、延时 500ms。

预期输出（每 5 个 A 出现 1 个 B）：

```
A A A A A B A A A A A B ...
```

**③ 改成生产者 / 消费者**

`sinTask` 每 10ms 算一次正弦目标值写入 `g_target`，`defaultTask` 每 100ms 打印一次：

```
target=0.0
target=30.9
target=58.8
target=80.9
target=95.1
target=95.1
target=80.9
...
```

> **注**：`g_target` 被两个任务读写，**必须加 `volatile`**。
> 生产快（100Hz）、消费慢（10Hz），和真实 PID 场景一致。
