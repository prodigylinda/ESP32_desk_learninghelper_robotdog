# Servo Dog Control 示例

在 `components/servo_dog_ctrl` 组件里已写好包括站立、趴下、前进、后退、左转、右转、前趴、后仰、左右扭动等动作的实现函数，并通过 `servo_dog_ctrl_task` 实现对舵狗的命令控制。

在控制是，只需要通过消息队列给 `servo_dog_ctrl_task` 发送 `dog_action_msg_t` 类型的消息即可，消息结构如下：
```
dog_action_msg_t msg = {
    .state = s_dog_state,
    .repeat_count = 2,
    .speed = 100,
    .hold_time_ms = 500,
    .angle_offset = 20
};
```
消息结构成员含义如下
```
typedef struct {
    servo_dog_state_t state;      // 动作类型，如趴下、前进、后退等
    uint16_t repeat_count;        // 动作重复次数
    uint16_t speed;               // 动作执行速度
    uint16_t hold_time_ms;        // 动作保持时间，在前趴、后仰动作中使用
    uint8_t angle_offset;         // 转到角度，在左右扭动动作中使用
} dog_action_msg_t;
```

具体使用请参考 `main/servo_dog.c` 里的 `button_single_click_cb` 函数用法。
