## 介绍

JQHttpServer是基于Qt开发的轻量级HTTP服务器，目前支持GET和POST两个协议。

#### 用到的Qt库有：

* core
* network
* concurrent	
* testlib（测试用，运行不需要）

理论上可以部署到任何Qt支持的平台上。

推荐使用Linux系统或者Unix系统，因为在5.6后，Qt更换了Unix相关系统的底层模型，从select更换为了poll，这样子网络库的并发就脱离了1024个的限制。

使用本库，需要 Qt5.0或者更高版本，以及支持 C++14 的编译器，对操作系统无要求。

本库源码均已开源在了GitHub上。

GitHub地址：https://github.com/188080501/JQHttpServer

本库的授权协议是：随便用

方便的话，帮我点个星星，或者反馈一下使用意见，这是对我莫大的帮助。

若你已经有了更好的建议，或者想要一些新功能，可以直接邮件我，我的邮箱是：Jason@JasonServer.com

或者直接在GitHub上提交问题：
https://github.com/188080501/JQHttpServer/issues

## 开发计划

阶段|日期
---|---
初始版本|已完成

## 性能介绍

本库性能只能说一般般，底层是poll注定了性能不是强项，以下是我在我电脑（ MacBookPro + 127.0.0.1 ）上，测出的性能。

QPS：
