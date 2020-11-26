# FTPClient
FTP客户端，用于自动下载服务器上某个文件夹中的内容
为了将FTP服务器上的某个文件夹里所有内容下载到本地，自己实现了FTP客户端的部分功能，初次尝试Socket编程，不是很规范，做份笔记，便于以后完善功能。

## 关于FTP

FTP协议包括两个组成部分，其一为FTP服务器，其二为FTP客户端。其中FTP服务器用来存储文件，用户可以使用FTP客户端通过FTP协议访问位于FTP服务器上的资源。

默认情况下FTP协议使用TCP端口中的 20和21这两个端口，其中20用于传输数据，21用于传输控制信息。

FTP支持两种模式，一种方式叫做Standard (也就是 PORT方式，主动方式)，一种是 Passive(也就是PASV，被动方式)。 Standard模式 FTP的客户端发送 PORT 命令到FTP服务器。Passive模式FTP的客户端发送 PASV命令到 FTP  Server。

**Port**

FTP 客户端首先和FTP服务器的TCP 21端口建立连接，通过这个通道发送命令，客户端需要接收数据的时候在这个通道上发送PORT命令。  PORT命令包含了客户端用什么端口接收数据。在传送数据的时候，服务器端通过自己的TCP 20端口连接至客户端的指定端口发送数据。 FTP  server必须和客户端建立一个新的连接用来传送数据。

**Passive**

在建立控制通道的时候和Standard模式类似，但建立连接后发送的不是Port命令，而是Pasv命令。FTP服务器收到Pasv命令后，随机打开一个高端端口（端口号大于1024）并且通知客户端在这个端口上传送数据的请求，客户端连接FTP服务器此端口，然后FTP服务器将通过这个端口进行数据的传送，这个时候FTP server不再需要建立一个新的和客户端之间的连接。

很多防火墙在设置的时候都是不允许接受外部发起的连接的，所以许多位于防火墙后或内网的FTP服务器不支持PASV模式，因为客户端无法穿过防火墙打开FTP服务器的高端端口；而许多内网的客户端不能用PORT模式登陆FTP服务器，因为从服务器的TCP 20无法和内部网络的客户端建立一个新的连接，造成无法工作。

## 框架

算法流程

```
输入：FTP服务器IP地址，用户名user，口令pwd，待下载的目标文件夹tarDir，本地保存位置localDir
流程：
1. 登录服务器，令ftpDir=tarDir；
2. 将ftp服务器上的工作目录切换到文件夹ftpDir；
3. 遍历ftp服务器工作目录中的所有文件（文件夹），若是文件，判断本地是否存在大小相同的同名文件，不存在就下载到本地；若是文件夹，令ftpDir=文件夹名，执行第2步（递归）；
4. 遍历结束，将ftp服务器上的工作目录切换到上一级目录。
```

## FTP命令

+ 发送用户名`USER PC\r\n`
+ 发送口令`PASS ***\r\n`
+ 切换目录`CWD dir\r\n`
+ 列出文件信息`LIST\r\n`
+ 进入被动模式`PASV\r\n`
+ 获取文件大小`SIZE filename\r\n`
+ 下载文件`RETR filename\r\n`
+ 设置服务器字符编码方式为utf-8`opts utf8 on\r\n`

## 递归遍历文件夹并下载

```c++
int FTPClient::FileIterator(string localDir, string ftpDir)
{
    //创建本地文件夹
    vector<string> vec_str = StringSplit(ftpDir, "/");
    string childDirName = vec_str[vec_str.size()-1];
    if(CreatChildDir(localDir, childDirName) == -1)
    {
        fout << "can not creat directory" << childDirName << " at localhost" << endl;
        return -1;
    }
    //ftp切换到目标文件夹
    ChangeDirectory(ftpDir);
	//获取当前目录信息
    string str_dir_info = ListDirectory();  
    vector<string> vec_dir_info = StringSplit(str_dir_info, "\r\n");
    FileNode *pre = nullptr;
    //遍历当前目录
    for(int i=0;i<vec_dir_info.size();++i)
    {
        string filename = vec_dir_info[i].substr(39, vec_dir_info[i].size() - 39);
        //若是文件夹，递归调用FileIterator
        if(vec_dir_info[i].substr(24,5) == "<DIR>")
        {
            FileIterator(localDir+"//"+childDirName,filename);
        }
        else//若是文件，调用下载函数
        {
            DownloadFile(localDir+"//"+childDirName, filename);
        }
    }
    //令ftp服务器工作目录恢复至调用FileIterator前的状态
    vector<string> vec_str_tmp = StringSplit(ftpDir, "/");
    for(int i=0;i<vec_str_tmp.size();++i)
        ChangeDirectory("..");
}
```

## 数据连接

当向FTP服务器发送`LIST\r\n`或`RETR filename\r\n`命令时，FTP服务器通过数据连接(令一个tcp连接)给出响应。这里，我采用被动方式建立数据连接。每次LIST或RETR时，都需要建立数据连接，完成数据传输后需要关闭数据连接。

```c++
int FTPClient::CreatDataLink()
{
    //进入被动模式
    fout << ">ftp PASV"  << "  ";//<< endl;
    this->DataSocket = socket(AF_INET, SOCK_STREAM, 0);
    string pasv_cmd = "PASV\r\n";
    send(this->ConnSocket, pasv_cmd.c_str(), pasv_cmd.size(), 0);
    string servresponse = GetResponse(this->ConnSocket);
    if(CheckResponse(servresponse, 227) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }

    //获取服务器数据链接开放端口
    int beg = servresponse.find_first_of('(');
    int end = servresponse.find_first_of(')');
    vector<int> vec_ip_port;
    vector<string> vec_str = StringSplit(servresponse.substr(beg+1, end - beg - 1), ",");
    for(int i=0;i<vec_str.size();++i) vec_ip_port.push_back(stoi(vec_str[i]));
    this->DataIP = to_string(vec_ip_port[0]) + "." + to_string(vec_ip_port[1]) + "." + to_string(vec_ip_port[2]) + "." + to_string(vec_ip_port[3]);
    this->DataPort = vec_ip_port[4]*256+vec_ip_port[5];
	//建立数据连接
    if(ConnectFTPServer(this->DataSocket, this->DataIP, this->DataPort) == -1)
        return -1;

    return 1;
}
```

## 解决中文编码问题

将服务器字符编码设置为utf8，可以解决中文无法识别的问题

```c++
int FTPClient::SetUTF8()
{
    fout << ">ftp opts utf8 on"  << "  ";//<< endl;
    string utf8_cmd = "opts utf8 on\r\n";
    send(this->ConnSocket, utf8_cmd.c_str(), utf8_cmd.size(),0);
    string servresponse = GetResponse(this->ConnSocket);
    if(CheckResponse(servresponse, 200) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }
}
```

## 获取服务器响应

向FTP服务器发送命令后，服务器会给出响应用于指示命令执行情况，如果有数据需要传输，还会通过数据连接发送数据，因此需要服务器回传的各种结果。

需要注意的是，服务器并没有用于指示数据发送完毕的标识，当服务器有大量的数据需要传输时，客户端需要反复读取，直至无法读取数据。对于数据发送何时结束的问题，采用了两个办法来解决，超时和判断接收数据的长度，当发生超时时，判断数据发送完成；当接收数据小于100时，判断数据发送完成（数据连接发送的数据有时不会填满整个缓冲区，不能简单根据接收数据是否填满整个缓冲区来判断后续是否还有数据，若只要接收到数据就继续接收直至超时，效率太低。考虑到命令执行的响应结果通常较短，因此当接收数据长度大于100时继续接收数据）

```c++
string FTPClient::GetResponse(int sockfd)
{
    int nRet = -1;
	char buf[2049] = {0};
    
    string strResponse;
	strResponse.clear();
    int len = 0;
    do
    {
        memset(buf,0,2049);
        len = recv(sockfd, buf,2048,0);
        strResponse += buf;
        //fout << len << endl;
    } while (len == 2048 || len > 100);
    
    return strResponse;
}
```

## Linux平台迁移到Windows平台

两个平台上socket用到的头文件不太一致，可以使用如下方式。

用到的链接库不同，超时时间不同

```c++
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wsock32.lib")
#define close closesocket 
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
typedef SOCKET int;
#endif
#include <string>
#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <dirent.h>
#include<algorithm>

using namespace std;
#ifdef WIN32
int timeout = 3000;
#else
struct timeval timeout = {3,0}; 
#endif
```

vscode的配置文件tasks.json中args参数中添加 `"-lwsock32"`，`"-lws2_32"`，`"-static"`；

vscode的配置文件tasks.json中type参数应该是shell

