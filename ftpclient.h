#ifndef FTPCLIENT_H
#define FTPCLIENT_H
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <direct.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wsock32.lib")
#define closesocket close
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

class FTPClient
{
public:
    FTPClient();
    ~FTPClient();
    int LoginFTPServer(string serverip, int serverport, string username, string pass);
    int CreatDataLink();
    int CloseDataLink();
    int DownloadFile(string localDir, string filename);
    int SetUTF8();
    int CreatChildDir(string fatherDirPath, string childDirName);
    int AutoDownloadDir(string localDir, string ftpDir);
    int CheckResponse(string response, int code);
    void ChangeDirectory(string dirpath);
    string PrintWorkDir();
    string ListDirectory();
    int FileIterator(string localdir, string ftpdir);
    
private:
    int ConnectFTPServer(int socketfd, string ip, int port);
    string GetResponse(int sockfd);
    vector<string> StringSplit(string strSrc, string strFlag);
    
private:
    SOCKET ConnSocket;
    SOCKET DataSocket;
    string ServerIP, DataIP, UserName, Password;
    int ServerPort, DataPort;
    ofstream fout;
};

#endif