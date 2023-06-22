# CN自己的主服务器 使用C++编写

还是有些bug的

* 服务器离线后可能不会从服务器列表删除
* 尚无私人房间功能

###### 编译

你需要以下库才能编译

boost(包括boost asio)
aisonlohmann json
openssl-win64

关于这些库 我推荐你用vcpkg来安装
详见[这里](https://vcpkg.io)

然后打开项目文件夹里的sln文件就可以了

目前是开发早期
