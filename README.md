# Windows平台的录音推流程序
使用winmm.lib库获取麦克风的PCM格式音频信号，调用fdk-aac库将PCM编码为AAC(ADTS)数据，通过srs-librtmp把数据推流到服务器。可以使用vlc等拉流工具进行接收。

## 默认参数
采样率：44100 hz
采样位宽：16 bit
通道：1
AAC压缩后比特率：128000 bsp

## 编译
This project can built and executed on your desktop with cmake:
```
cd audio_rtmp
mkdir build
cd build
cmake ..
make
```

## TODO
该程序为esp32窃听器的测试程序，后期会将该程序移植到esp32制作一个小型化的窃听器，完成后我会将代码和硬件设计上传。