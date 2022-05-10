## 介绍

JQHttpServer是基于Qt开发的轻量级HTTP/HTTPS服务器

底层有QTcpSocket、QSslSocket两个版本，分别对应HTTP和HTTPS。

#### 用到的Qt库有：

* core
* network
* concurrent
* testlib（测试用，运行不需要）
* OpenSSL（如果需要HTTPS）

不依赖外部库，理论上可以部署到任何Qt支持的平台上。

推荐使用Linux系统或者Unix系统，因为在5.7后，Qt更换了Unix相关系统的底层模型，从select更换为了poll，这样改进后，并发就脱离了1024个的限制。

使用本库，推荐 Qt5.8.0 或者更高版本，以及支持 C++11 的编译器（例如VS2013或者更高），对操作系统无要求。

本库源码均已开源在了GitHub上。

GitHub地址：https://github.com/188080501/JQHttpServer

方便的话，帮我点个星星，或者反馈一下使用意见，这是对我莫大的帮助。

若你遇到问题、有了更好的建议或者想要一些新功能，都可以直接在GitHub上提交Issues：https://github.com/188080501/JQHttpServer/issues

## 性能介绍

本库性能一般，符合一般项目使用标准

原因是Qt底层是poll，库中又有一些跨线程操作

在我的电脑（ MacBookPro 16" & i9 CPU & macOS 10.15.7 ）使用siege进行测试，命令行参数如下：

```siege -c 2 -r 5000 http://127.0.0.1:23412```

结果如下：
```
{	"transactions":			       10000,
	"availability":			      100.00,
	"elapsed_time":			        1.24,
	"data_transferred":		        0.10,
	"response_time":		        0.00,
	"transaction_rate":		     8064.52,
	"throughput":			        0.08,
	"concurrency":			        1.90,
	"successful_transactions":	       10000,
	"failed_transactions":		           0,
	"longest_transaction":		        0.35,
	"shortest_transaction":		        0.00
}
```

即QPS 8064


## License

See the [LICENSE](LICENSE.txt) file for license rights and limitations (MIT).
