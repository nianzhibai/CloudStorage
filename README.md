Son of bitch !!!

这是一个用C++写的云盘项目，你最好有点编程基础，因为项目做的交互一般。
如果使用到出现问题，提issue😎

### 1.配置环境

1. 确认系统类型

   ```cat /etc/os-release```查看是什么系统

2. 安装Git（这个你应该有吧😡）

   👉 Ubuntu / Debian 系列：

   ```bash
   sudo apt update apt
   sudo install git -y
   ```

   👉 CentOS / RHEL 系列：

   ```bash
   sudo yum update -y
   sudo yum install git -y
   ```

   👉 Rocky / AlmaLinux（新版 RHEL 系）：

   ```bash
   sudo dnf update -y
   sudo dnf install git -y
   ```

3. 安装完整的构建工具包（包含 gcc、g++、make 等）

   👉 Ubuntu / Debian 系列

   ```bash
   sudo apt update apt
   sudo apt install build-essential -y
   ```

   👉 CentOS / RHEL / Rocky Linux / AlmaLinux

   ```bash
   sudo yum update -y
   sudo yum groupinstall "Development Tools" -y
   ```

### 2.克隆项目

```bash
git clone https://github.com/nianzhibai/CloudStorage.git
```

### 3.使用这个云盘

1. 项目目录

   1. 这个项目的CloudStorage目录存放的是项目的主要代码，Tes目录存放的是作者写代码测试时用的。
   2. CloudStorage目录中
      1. CloudStorageClient目录，客户端代码的实现。
      2. CloudStorageServer目录，服务器代码。这里的服务器代码没有啥，因为服务器核心代码主要实现在一个个头文件中。
      3. Log目录，日志器实现。项目没有支持日志落地到文件，日志直接显示在屏幕上。
      4. TcpServer目录，服务器代码主要实现地方，存放很多服务器要用到的头文件。
      5. UserFile目录，UserFile目录下还会为每个用户创建一个自己的目录，存放每个用户上传的文件。

2. 启动云盘服务器和客户端

   1. 进入服务器CloudStorageServer目录，编译代码并执行。

      ```bash
      # CloudStorageServer目录并编译代码
      cd CloudStorage/CloudStorageServer #注意路径问题，你懂的
      make clean;make
      
      # 启动云盘服务器
      ./CloudStorageServer
      ```

      最开始讲的这个项目的交互不是很好，对于服务器不用没有这回事，服务器很ok。

   2. 进入客户端CloudStorageClient目录，修改代码、编译、执行。

      1. 修改代码

         1. 因为客户端的套接字部分，客户端连接服务器的ip是写死的，也没有进行交互式更新，所以我们要改下这个代码。

         2. vim打开CloudstorageClient.cpp，定位到617行，有如下代码

            ```c++
            inet_aton("127.0.0.1", &(server_addr.sin_addr));
            ```

            将 `127.0.0.1` 改成你云盘服务器IP即可，其他都不用改。

      2. 编译并执行

         ```bash
         make clean;make
         ./CloudStorageClient
         ```

      3. 客户端链接到服务器会显示云盘菜单，选择对应的即可

3. 注意

   1. 你上传到服务器的文件默认在 `CloudStorage/UsersFile/Su` 目录下，如果你想改变目录。你首先在UserFile目录创建对应的目录，然后将客户端CloudStorageClient.cpp第16行的

      ```c++
      std::string user_base_dir = "Su/";
      // 改成
      std::string user_base_dir = "你创建的目录名/";
      ```

      然后你的文件就回放到你指定的目录下。

   2. 如果你想多人用这个云盘，你就要给没个人创建一个目录，然后修改客户端的第16行代码，然后就可以用了，服务端是支持多人使用的。

   3. 如果你没有上面的需求，就不用改。

### 完结🎉

如果你感觉代码帮到你，给个star✨
