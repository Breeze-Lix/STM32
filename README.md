# FreeRTOS_and_PID

> 本工程关于学习 FreeROTS 的内容,同时进行 PID 的学习

## FreeRTOS 概念

暂无



## 接口一览

|接口|地点|
|PA0/PA1|电机A/B相|
|PA2/PA3|URAST2_Tx/Rx|
|PB1/PB0|AIN1/AIN2|
|PWMA|PB6|


## 1. 新建freerots工程

**新建CubeMX文件并设置：**

**1.** 在 "Middleware and Software Packs" 里面找 FreeRTOS 
**2.** 设置 Interface -> CMSIS_V1
**3.** 设置 Stack Size -> 256
**4.** 在 "System Core" 里面找 SYS ,设置 Timebase Source -> TIM3
**5.** 生成代码用板载灯闪灯检验，位置 void StartDefaultTask -> for 循环

**注：** 记得开启 USART2 （不用DMA\NVIC）和加入 fputc ，可用来测试


## 2. 任务并行

