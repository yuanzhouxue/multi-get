使用socks5代理时，客户端先需要与代理服务器进行连接、认证，一切完毕之后，客户端即可通过代理服务器访问网络。

## 与代理服务器建立连接

要与代理服务器进行连接，客户端首先需要指明协议的版本及认证方式。发送的报文格式如下：

|   字段   | 字节数 |
| :------: | :----: |
|   VER    |   1    |
| NMETHODS |   1    |
| METHODS  | 1-255  |

- VER指定协议版本，这里填0x05
- NMETHODS表示客户端支持多少种认证方式
- METHODS表示客户端支持的认证方式，可以有多种。简单起见，本项目只实现了不需要认证的方式，值为0x00。

代理服务器收到请求后发出响应，格式为：

|  字段  | 字节数 |
| :----: | :----: |
|  VER   |   1    |
| METHOD |   1    |

代理服务器从客户端支持的认证方式中选一种出来，即METHOD。因为这里选择的是免认证的授权方式，无需再发送认证报文，可以直接与远程服务器建立连接了。

## 建立连接

连接又客户端发起，告诉代理服务器客户端需要访问哪个远程服务器，包括地址和端口号。发送的报文格式为：

|   字段   |  字节数  |
| :------: | :------: |
|   VER    |    1     |
|   CMD    |    1     |
|   RSV    |    1     |
|   ATYP   |    1     |
| DST.ADDR | Variable |
| DST.PORT |    2     |

- VER表示谢意版本
- CMD代表客户端请求的类型，有3种取值：0x01：CONNECT，0x02：BIND，0x03：UDP ASSOCIATE
- RSV保留字，只能取0x00
- ATYP表示远程服务器的地址类型，0x01为IPv4地址，0x03为域名，0x04为IPv6地址
- DST.ADDR表示远程服务器的地址，长度不定
- DST.PORT表示远程服务器的端口，2字节

代理服务器收到请求后，会根据远程服务器的地址和端口尝试建立连接，不管连接是否成功，会给客户端发送响应，格式为：

|   字段    |  字节数  |
| :-------: | :------: |
|    VER    |    1     |
|    REP    |    1     |
|    RSV    |    1     |
|   ATYP    |    1     |
| BIND.ADDR | Variable |
| BIND.PORT |    2     |

其中REP代表响应状态码，为0x00时表示连接成功。

建立连接成功后，客户端即可通过与代理服务器之间建立的TCP连接和远程服务器进行通信了。



代码中socks5代理连接在`Connection::do_proxy_handshake()`中。