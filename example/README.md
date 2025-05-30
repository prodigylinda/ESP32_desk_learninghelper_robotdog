# ESP-Hi 示例

此目录包含一系列 ESP-Hi 示例项目。这些示例旨在演示模块的部分功能

ESP-Hi 的初始化代码位于 [components](../components) 文件夹。若移动工程地址，请修改例程中的 CMakeLists 引入组件的相对路径

# 示例列表
- [factory_bin](./factory_bin): 编译好的小智 LLM 语音交互固件，可直接烧录到开发板，源码已合入到：[xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)
- [web_control](./web_control): 通过网页界面可以远程控制舵机狗的运动和进行舵机校准
- [servo_dog](./servo_dog): 实现对舵狗的命令控制
- [audio_loop_test](./audio_loop_test): 通过按下按钮来录制音频，松开按钮后播放录制的音频
- [wake_word_test](./wake_word_test): 测试唤醒词
- [lcd_audio](./lcd_audio): 同时处理显示和音频
- [music_player](./music_player): 多首音频文件的本地循环播放
- [sensor_test](./sensor_test): 测试连接不同的外设