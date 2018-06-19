## 介绍

JQHttpServer是基于Qt开发的轻量级HTTP/HTTPS服务器，目前支持GET和POST两个协议。

底层有QTcpSocket、QSslSocket和QLocalSocket三个版本，方便使用。

#### 用到的Qt库有：

* core
* network
* concurrent	
* testlib（测试用，运行不需要）
* OpenSSL（如果需要HTTPS）

理论上可以部署到任何Qt支持的平台上。

推荐使用Linux系统或者Unix系统，因为在5.7后，Qt更换了Unix相关系统的底层模型，从select更换为了poll，这样改进后，并发就脱离了1024个的限制。

使用本库，推荐 Qt5.7.0 或者更高版本，以及支持 C++11 的编译器（例如VS2013或者更高），对操作系统无要求。

本库源码均已开源在了GitHub上。

GitHub地址：https://github.com/188080501/JQHttpServer

方便的话，帮我点个星星，或者反馈一下使用意见，这是对我莫大的帮助。

若你遇到问题、有了更好的建议或者想要一些新功能，都可以直接在GitHub上提交Issues：https://github.com/188080501/JQHttpServer/issues

## 性能介绍

本库性能只能说一般般，底层是poll，而且又有一些跨线程操作。

在我的电脑（ iMac + 127.0.0.1 ）上，HTTP的QPS为1670。
