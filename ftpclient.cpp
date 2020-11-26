#include "ftpclient.h"

FTPClient::FTPClient()
{
    
    #ifdef WIN32
    WORD sockVersion = MAKEWORD(2,2);  
    WSADATA wsaData;  
    if(WSAStartup(sockVersion, &wsaData)!=0)  
    {  
        cout << "err" << endl; 
    }
    this->ConnSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(this->ConnSocket,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(timeout));
    setsockopt(this->ConnSocket,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(timeout));
    #else
    this->ConnSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(this->ConnSocket,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(struct timeval));
    setsockopt(this->ConnSocket,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
    #endif
    fout.open("log.txt");
    
    
}

FTPClient::~FTPClient()
{
    
    //还需要关闭FTP连接
    //...
    close(this->ConnSocket);
    //close(this->DataSocket);
    fout.close();
    
}

int FTPClient::LoginFTPServer(string serverip, int serverport, string username, string pass)
{
    this->ServerIP = serverip;
    this->ServerPort = serverport;
    this->UserName = username;
    this->Password = pass;
    fout << "Connectting to FTP Server: " << this->ServerIP << endl;

    if(ConnectFTPServer(this->ConnSocket, this->ServerIP, this->ServerPort) == -1)
        return -1;
    string servresponse = GetResponse(this->ConnSocket);
    fout << servresponse << endl;

    fout << ">ftp USER " << this->UserName << "  ";//<< endl;
    string user_cmd = "USER " + this->UserName + "\r\n";
    send(this->ConnSocket, user_cmd.c_str(), user_cmd.size(),0);
    servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    if(CheckResponse(servresponse, 331) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }
    fout << ">ftp PASS *"  << "  ";//<< endl;
    string pass_cmd = "PASS " + this->Password + "\r\n";
    send(this->ConnSocket, pass_cmd.c_str(), pass_cmd.size(), 0);
    servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    if(CheckResponse(servresponse, 230) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }
    return 1;
}
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
    
    //fout << servresponse << endl;
    //获取服务器数据链接开放端口
    
    int beg = servresponse.find_first_of('(');
    int end = servresponse.find_first_of(')');

    //fout << "creat data link" << endl;
    vector<int> vec_ip_port;
    vector<string> vec_str = StringSplit(servresponse.substr(beg+1, end - beg - 1), ",");
    for(int i=0;i<vec_str.size();++i) vec_ip_port.push_back(stoi(vec_str[i]));
    this->DataIP = to_string(vec_ip_port[0]) + "." + to_string(vec_ip_port[1]) + "." + to_string(vec_ip_port[2]) + "." + to_string(vec_ip_port[3]);
    this->DataPort = vec_ip_port[4]*256+vec_ip_port[5];

    if(ConnectFTPServer(this->DataSocket, this->DataIP, this->DataPort) == -1)
        return -1;
    //servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    return 1;
}
int FTPClient::CloseDataLink()
{
    //fout << "close data link" << endl;
    int r = close(this->DataSocket);
    string servresponse = GetResponse(this->ConnSocket);

   return r;
}
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
    //fout << endl;
    
    return strResponse;
}
int FTPClient::ConnectFTPServer(int sockfd, string ip, int port)
{
    struct sockaddr_in servaddr;
    unsigned int argp = 1;
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(sockaddr)) == -1)
    {
        fout << "can not connect to " << ip << endl;
        return -1;
    }
    return 1;
}
int FTPClient::DownloadFile(string localDir, string filename)
{
    fout << ">ftp SIZE " << filename << "  ";// << endl;
    string size_cmd  = "SIZE " + filename + "\r\n";
    send(this->ConnSocket, size_cmd.c_str(), size_cmd.size(), 0);
    string servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    if(CheckResponse(servresponse, 213) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
        return -1;
    }
    vector<string> vec_str = StringSplit(servresponse, " ");
    int filesize = stoi(vec_str[1]);

    //检查本地是否存在该文件，若文件大小一致视作相同，不必重新下载
    fstream fin(localDir+ "\\" + filename, ios::in | ios::binary);
    if(fin.is_open())
    {
        fin.seekg(0,ios::end);
        if(fin.tellg() == filesize)
        {
            fin.close();
            return 1;
        }
    }
    fin.close();

    
    
    CreatDataLink();
    cout << filename << "    ";
    fout << ">ftp RETR " << filename << "  ";// << endl;
    unsigned char *data = new unsigned char[filesize];
    string download_cmd = "RETR " + filename + "\r\n";
    send(this->ConnSocket, download_cmd.c_str(), download_cmd.size(), 0);
    servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    if(CheckResponse(servresponse, 125) == 1 || CheckResponse(servresponse, 150) == 1)
    {
        fout << "OK" << endl;
        cout << "OK" << endl;
    }
    else
    {
        fout << "Err" << endl;
        cout << "Err" << endl;
    }
    int r = recv(this->DataSocket, (char*)data, filesize, 0);
    fstream fout(localDir+ "\\" + filename, ios::out | ios::binary);
    fout.write((char*)data, filesize);
    fout.close();
    delete[] data;
    CloseDataLink();
    return r;
}
void FTPClient::ChangeDirectory(string dirpath)
{
    if(dirpath == "")
        return;
    fout << ">ftp " << "cd " + dirpath << "  ";// << endl;
    string cwd_cmd = "CWD " + dirpath + "\r\n";
    send(this->ConnSocket, cwd_cmd.c_str(), cwd_cmd.size(), 0);
    string servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    if(CheckResponse(servresponse, 250) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }
}
string FTPClient::ListDirectory()
{
    CreatDataLink();
    fout << ">ftp LIST  ";// << endl;
    string ls_cmd = "LIST\r\n";
    send(this->ConnSocket, ls_cmd.c_str(), ls_cmd.size(), 0);
    string servresponse = GetResponse(this->ConnSocket);
    //cout << servresponse << endl;
    if(CheckResponse(servresponse, 125) == 1 || CheckResponse(servresponse, 150) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }
    servresponse = GetResponse(this->DataSocket);
    fout << servresponse << endl;
    CloseDataLink();
    return servresponse;
}
string FTPClient::PrintWorkDir()
{
    fout << ">ftp PWD"  << "  ";//<< endl;
    string pwd_cmd = "PWD\r\n";
    send(this->ConnSocket, pwd_cmd.c_str(), pwd_cmd.size(),0);
    string servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    return servresponse;
}
vector<string> FTPClient::StringSplit(string strSrc, string strFlag)
{
    vector<string> vec_str;
    int beg(0), end(0);
    while(1)
    {
        end = strSrc.substr(beg, strSrc.size() - beg).find_first_of(strFlag) + beg;
        if(end < beg) break;
        string str_tmp = strSrc.substr(beg, end-beg);
        vec_str.push_back(str_tmp);
        beg = end + strFlag.size();
    }
    if(beg < strSrc.size())
        vec_str.push_back(strSrc.substr(beg, strSrc.size() - beg));
    return vec_str;
}
int FTPClient::FileIterator(string localDir, string ftpDir)
{
    //创建本地文件夹
    //cout << "ftpdir" << " " << ftpDir << endl;
    if(localDir.size() > 0 && localDir[localDir.size() - 1] == '\\')
        localDir = localDir.substr(0, localDir.size()-1);
    //cout << "localDir " << localDir << endl;
    vector<string> vec_str = StringSplit(ftpDir, "\\");
    string childDirName("");
    if(vec_str.size() > 0)
    {
        childDirName = vec_str[vec_str.size()-1];
        //cout << "childDirName" << " " << childDirName << endl;
        if(CreatChildDir(localDir, childDirName) == -1)
        {
            fout << "can not creat directory" << childDirName << " at localhost" << endl;
            return -1;
        }
    }
    
    //ftp切换到目标文件夹
    replace(ftpDir.begin(), ftpDir.end(), '\\', '/');
    ChangeDirectory(ftpDir);
    string str_dir_info = ListDirectory();
        
    vector<string> vec_dir_info = StringSplit(str_dir_info, "\r\n");
    vector<string> vec_str_tmp;
    for(int i=0;i<vec_dir_info.size();++i)
    {
        if(vec_dir_info[i][0] == 'd' || vec_dir_info[i][0] == '-')
        {
            string filename = vec_dir_info[i].substr(56, vec_dir_info[i].size() - 56);
            if(vec_dir_info[i][0] == 'd')
            {
                FileIterator(localDir+"\\"+childDirName,filename);
            }
            else
            {
                DownloadFile(localDir+"\\"+childDirName, filename);
            }
            vec_str_tmp = StringSplit(ftpDir, "/");
        }
        else if(vec_dir_info[i][0] >= '0' && vec_dir_info[i][0] <= '2')
        {
            string filename = vec_dir_info[i].substr(39, vec_dir_info[i].size() - 39);
            if(vec_dir_info[i].substr(24,5) == "<DIR>")
            {
                FileIterator(localDir+"\\"+childDirName,filename);
            }
            else
            {
                DownloadFile(localDir+"\\"+childDirName, filename);
            }
            vec_str_tmp = StringSplit(ftpDir, "\\");
        }
        else
        {
            fout << vec_dir_info[i] << endl;
            cout << "FTP Server's Operation System is not Windows or Linux" << endl;
            //return -1;
        }
        
    }
    //cout << ftpDir << endl;
    //vector<string> vec_str_tmp = StringSplit(ftpDir, "\\");
    //cout << vec_str_tmp.size() << endl;
    for(int i=0;i<vec_str_tmp.size();++i)
        ChangeDirectory("..");
    return 1;
}
int FTPClient::SetUTF8()
{
    fout << ">ftp opts utf8 on"  << "  ";//<< endl;
    string utf8_cmd = "opts utf8 on\r\n";
    send(this->ConnSocket, utf8_cmd.c_str(), utf8_cmd.size(),0);
    string servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    if(CheckResponse(servresponse, 200) == 1)
    {
        fout << "OK" << endl;
        return 1;
    }
    else
    {
        fout << "Err" << endl;
        return -1;
    }
}
int FTPClient::CreatChildDir(string fatherDirPath, string childDirName)
{
    string childDirPath = fatherDirPath + "\\" + childDirName;
    //cout << childDirPath << endl;
    if(access(fatherDirPath.c_str(), 0) == 0)
    {
        if(access(childDirPath.c_str(), 0) == 0 || mkdir(childDirPath.c_str()) == 0)
            return 1;   
    }
    return -1;
    
}
int FTPClient::CheckResponse(string response, int code)
{
    vector<string> vec_str = StringSplit(response, " ");
    if(vec_str.size() == 0)
        return -1;
    int r_code = stoi(vec_str[0]);
    if(r_code == code)
        return 1;
    else
    {
        return -1;
    }
    
}

int main(int argc, char *argv[])
{
    FTPClient ftp;
    ftp.LoginFTPServer(argv[1], 21, argv[2], argv[3]);
    ftp.SetUTF8();
    string localDir = argv[4];//"C:\\Users\\PC\\Desktop\\ftp";
    string tar_directory = argv[5];//"\\netcat-win32-1.12";
    if(localDir.size() > 0 && localDir[localDir.size() - 1] == '\\')
        localDir = localDir.substr(0, localDir.size()-1);
    if(tar_directory.size() > 0 && tar_directory[0] == '\\')
        tar_directory = tar_directory.substr(1, tar_directory.size()-1);
    while(1)
    {
        ftp.FileIterator(localDir, tar_directory);
        Sleep(10000);
    }
    
    
    return 0;
}
