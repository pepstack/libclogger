# clogger

clogger C API 是一个可以跨平台调试的 C语言静态库，动态库和调用程序的示例项目。示例项目由下面的命令行自动生成：

    $ cd pytools/tools/
    $ gen_project.py --project=clogger


Author: zhang

Date: 2021-07-06 17:43:56

Refer: https://blog.csdn.net/ubuntu64fan/article/details/106689478


## 可供跨平台调试的 C/C++ 程序项目

系统要求：

- Win10 家庭版 （Windows_NT x64 10.0.19042）
- VS2015 社区版 (C/C++)
- VSCode on Win10
        Version: 1.57.1
        Date: 2021-06-17T13:28:07.755Z
        Electron: 12.0.7
        Chrome: 89.0.4389.128
        Node.js: 14.16.0
        V8: 8.9.255.25-electron.0
        OS: Windows_NT x64 10.0.19042
        Extensions:
            Remote Development
            C/C++ Extension Pack
- msys2-x86_64-20210604.exe

### 在 Windows 上用 Visual Studio 调试代码

  用 VS2015 打开项目文件：msvc/clogger-ALL-vs2015.sln，然后编译调试即可。

### VSCode 整合 mingw 在 Windows 上调试代码

首先安装好 msys2 环境，参考：https://www.msys2.org/ 。或者如下过程：

- 下载：

        https://github.com/msys2/msys2-installer/releases/download/2021-06-04/msys2-x86_64-20210604.exe

- 安装开发工具链:

        $ pacman -Syu
    
        $ pacman -Su
    
        $ pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-i686-toolchain

然后根据用户的安装位置目录设置好如下环境变量（路径中不可用有空格、全角字符）：

    MSYS2_PATH_TYPE=inherit
    MSYS64_HOME=C:\DEVPACK\msys64
    MSYS64_ROOT_BASH=/C/DEVPACK/msys64
    WORKSPACE_ROOT_BASH=/C/Users/cheungmine/Workspace/github.com/pytools/gen-projects

添加如下的位置到 Path 环境变量：

    Path=...;%MSYS64_HOME%;%MSYS64_HOME%\usr\bin;C:\DEVPACK\MicrosoftVSCode;C:\DEVPACK\MicrosoftVSCode\bin;

根据代码所在的目录，务必创建一个驱动器链接："/C" -> "C:\"

    C:\>mklink /J "/C" "C:\"

如果你的代码在 E: 盘，则创建：

    C:\>mklink /J "/E" "E:\"

注意：关闭防病毒软件以加速编译和调试速度。

接下来就可以用 VSCode 打开项目的目录（.vscode 所在的目录）（或者打开cmd，进入到项目目录，输入："code ."）。

    C:\Users\cheungmine>cd C:\Users\cheungmine\Workspace\github.com\pytools\gen-projects\clogger

    C:\Users\cheungmine>cd C:\Users\cheungmine\Workspace\github.com\pytools\gen-projects\clogger>code .

在 VSCode 窗口，为源程序 app_main.c 设置断点，然后选择：

  Run and Debug: (gdb) mingw64 debug

就可以在 Windows 上用 VSCode 调试程序了。

## 在 Windows 上远程调试 Linux  服务器上的代码

远程 Linux 服务器版本：centos7 + gcc, gdb。

仍然使用 VSCode，将代码复制到 Linux 服务器上，用 VSCode 远程连接到服务器，打开项目目录。按如下选择：

  Run and Debug: (gdb) linux64 debug

就可以在 Windows 上用 VSCode 调试 Linux 服务器上的程序了。

此时仍需要输入密码，下面配置免密：

打开 cmd 命令行窗口，生成密钥对：

      C:\> cd %USERPROFILE%
      C:\Users\cheungmine\>ssh-keygen

然后把 C:\Users\cheungmine\.ssh\id_rsa.pub 复制到 Linux 上，我用 msys shell:

    $ ssh-copy-id -i /C/Users/cheungmine/.ssh/id_rsa.pub root@node1

然后在 cmd 中测试是否可用免密登录到 node1:

    C:\Users\cheungmine\>ssh root@node1

最后打开 VSCode，配置远程文件：C:\Users\cheungmine\.ssh\config, 如下：

    # Read more about SSH config files: https://linux.die.net/man/5/ssh_config
    Host node1
        HostName node1
        User root

此时就可用远程免密登录 Linux 服务器，编译和调试远程项目了。切记不要配置：IdentifyFile ！

## 只构建代码（不调试）

进入项目目录（Makefile 所在的目录）可用直接 make，就编译出完整的项目了。支持的 shell 有：

- cygwin (Windows)

- mingw (Windows)

- bash (Linux)

按 "make help" 显示编译帮助。