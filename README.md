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

**1.** FreeRTOS中 Tasks and Queues -> Tasks -> Add ->
       |参数|填写|
       |Task Name|sinTask|
       |Priority|osPriorityNormal|
       |Stack Size|256|
       |Entry Function|StartSinTask|
**2.** 在 default 和 sin 分别安排打印 'A'/'B',延时 100/500,测试并发
       结果：A A A A A B A A A A A B ...
**3.** 测试正弦任务生成目标值 sin -> 生产者 default -> 消费者
```
结果 （有点误差）
target=0.0
target=30.9
target=58.8
target=80.9
target=95.1
target=95.1
target=80.9
...
```

**注：** 两个函数读取，target 要 volatile 
         生产快、消费慢，模拟真实PID

