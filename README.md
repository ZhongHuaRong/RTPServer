# RTPServer
搭配rtplivelib使用的服务器应用
简单实现了一些基本功能，很多地方没有妥善处理，存在较多bug

## 开发环境的搭建
1.虽然是纯C++项目，但是我没有编写MakeFile文件，所以使用qmake。推荐安装IDE（QtCreator），推荐把整个Qt SDK下载下来，链接：https://download.qt.io/official_releases/qt/<br>
  编译的时候,Windows系统推荐Mingw 64bit,Linux系统使用GCC 64bit,不要使用32bit<br>
2.打开QtCreator，打开项目，构建即可生成可执行文件

## 部署
部署就比较简单了，直接上传可执行文件到服务器，然后将sdk/lib目录下的那几个.so文件上传到服务器的/usr/lib目录，然后执行可执行文件即可

