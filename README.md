# WebStomp++
Stomp Support at the basis of WebSocketpp
基于WebSocket++的Stomp协议框架

## 框架介绍
- WebStompClient: Stomp协议客户端
- WebStompServer: Stomp协议服务端(doing)

## 依赖库
- boost: https://www.boost.org/
- WebSocket++: https://github.com/zaphoyd/websocketpp

## 接口介绍
### WebClient类
- `WebStompClient()`: 初始化Stomp客户端
- `Connect(uri)`: 连接至指定的Stomp服务端点
- `Disconnect()`: 关闭Stomp连接
- `Run()`: 运行客户端服务(阻塞)当连接关闭后该函数会返回
- `Subscribe(destination, callback)`: 订阅指定位置并注册该订阅广播的回调
  - 回调函数统一为`std::function<void(webstomppp::StompCallbackMsg)>`类型的可调用对象，用户可根据需要传入具体的可调用类型
- `Send(send_msg)`: 向服务端发送一条消息
- `virtual OnConnected()`: 连接成功时回调
- `virtual void OnDisconnected()`: 断开连接时回调

### Frame封装
WebStomppp提供了一些封装的帧模板，可以直接使用
- `StompFrame`: 不含任何命令类型的普通Frame
- `StompSendFrame`: Send命令的Frame
- `StompJsonSendFrame`: Send命令, 内容类型为application/json的Frame
- `StompTextSendFrame`: Send命令, 内容类型为plain/text的Frame
- `StompSubscribeFrame`: Subscribe命令的Frame

## How to build
`MSVC`: Visual Studio

`UNIX`:  
```shell
cmake .
make
```
# Todo List
- 服务端
- GNU Support
