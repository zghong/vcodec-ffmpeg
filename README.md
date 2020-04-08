# READEME.md

## 说明

本项目使用`ffmpeg`对常见音视频编解码，已经集成的第三方编解码有`libx264, libx265, libopus, libspeex, libfdk-aac`，相关开发文件位于`./include, ./lib`。

已经编写测试通过的编解码方案有`h264, h265`。

## 编译与运行

`g++ main.cpp -I ../include/ -L ../lib/ -lavcodec -lavutil`
