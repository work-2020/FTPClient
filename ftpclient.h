#ifndef FTPCLIENT_H
#define FTPCLIENT_H
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
using namespace std;

class FileNode
{
public:
    char * name;
    FileNode * next;
    FileNode * father;
    FileNode * child;
    
public:  
    FileNode(char* filename)
    {
        this->name = filename;
        //name = ptr_c;
        this->next = nullptr;
        this->father = nullptr;
        this->child = nullptr;
    }
    ~FileNode()
    {
        delete[] name;
        this->next = nullptr;
        this->father = nullptr;
        this->child = nullptr;
    }
   /* 
public:
    void SetName(string filename)
    {
        
        memcpy(ptr_c, filename.c_str(), filename.size());
        //this->name = ptr_c;
    }
    */
    //需要析构函数
    //...
};

struct timeval timeout = {3,0}; 

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
    int ConnSocket;
    int DataSocket;
    string ServerIP, DataIP, UserName, Password;
    int ServerPort, DataPort;
    FileNode * iter_dir;
    ofstream fout;
};

#endif