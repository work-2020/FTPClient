#include "ftpclient.h"

FTPClient::FTPClient()
{
    this->ConnSocket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(this->ConnSocket,SOL_SOCKET,SO_SNDTIMEO,(char *)&timeout,sizeof(struct timeval));
    setsockopt(this->ConnSocket,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
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

/*
    struct sockaddr_in servaddr;
    unsigned int argp = 1;
    //ioctl(this->ConnSocket, FIONBIO, &argp);//设置为非阻塞模式
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(this->ServerPort);
    servaddr.sin_addr.s_addr = inet_addr(this->ServerIP.c_str());

    if (connect(this->ConnSocket, (struct sockaddr *)&servaddr, sizeof(sockaddr)) == -1)
    {
        fout << "can not connect to " << this->ServerIP << endl;
        return -1;
    }
*/
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
    /*
    string str_ip_port = servresponse.substr(beg+1, end - beg - 1);
    vector<int> vec_ip_port;
    beg = 0;
    while(1)
    {
        end = str_ip_port.substr(beg, str_ip_port.size() - beg).find_first_of(',') + beg;
        if(end < beg) break;
        string str_tmp = str_ip_port.substr(beg, end-beg);
        vec_ip_port.push_back(stoi(str_tmp));
        beg = end + 1;
    }
    vec_ip_port.push_back(stoi(str_ip_port.substr(beg, str_ip_port.size() - beg)));
    //for(int i=0;i<vec_ip_port.size();++i) fout << vec_ip_port[i] << endl;
    */
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
    close(this->DataSocket);
    string servresponse = GetResponse(this->ConnSocket);
    /*
    fout << servresponse << endl;
    if(CheckResponse(servresponse, 226) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }
    */
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
    fstream fin(localDir+ "//" + filename, ios::in | ios::binary);
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
    fout << ">ftp RETR " << filename << "  ";// << endl;
    unsigned char *data = new unsigned char[filesize];
    string download_cmd = "RETR " + filename + "\r\n";
    send(this->ConnSocket, download_cmd.c_str(), download_cmd.size(), 0);
    servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    if(CheckResponse(servresponse, 125) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }
    recv(this->DataSocket, data, filesize, 0);
    fstream fout(localDir+ "//" + filename, ios::out | ios::binary);
    fout.write((char*)data, filesize);
    fout.close();
    delete[] data;
    CloseDataLink();
}
void FTPClient::ChangeDirectory(string dirpath)
{
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
    if(CheckResponse(servresponse, 125) == 1)
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
    vector<string> vec_str = StringSplit(ftpDir, "/");
    string childDirName = vec_str[vec_str.size()-1];
    if(CreatChildDir(localDir, childDirName) == -1)
    {
        fout << "can not creat directory" << childDirName << " at localhost" << endl;
        return -1;
    }
    //ftp切换到目标文件夹
    //fout << "Dir " << ftpDir << endl; 
    ChangeDirectory(ftpDir);
    //string workdir = PrintWorkDir();
    //fout << "workdir: " << workdir << endl;
    string str_dir_info = ListDirectory();
        
    vector<string> vec_dir_info = StringSplit(str_dir_info, "\r\n");
    FileNode *pre = nullptr;
    
    for(int i=0;i<vec_dir_info.size();++i)
    {
        string filename = vec_dir_info[i].substr(39, vec_dir_info[i].size() - 39);
        /*
        char *ptr_c = new char[filename.size() + 1];
        memcpy(ptr_c, filename.c_str(), filename.size());
        ptr_c[filename.size()] = 0;
        FileNode *child = new FileNode(ptr_c);
        child->father = father;
        if(i == 0)
            father->child = child;
        else
        {
            pre->next = child;
        }
        */
        if(vec_dir_info[i].substr(24,5) == "<DIR>")
        {
            FileIterator(localDir+"//"+childDirName,filename);
        }
        else
        {
            DownloadFile(localDir+"//"+childDirName, filename);
        }
        
        /*
        if(pre != nullptr) fout << "pre: " << pre->name << " ";
        fout << "cur: " << child->name << endl; 
        pre = child;
        */
    }
    vector<string> vec_str_tmp = StringSplit(ftpDir, "/");
    for(int i=0;i<vec_str_tmp.size();++i)
        ChangeDirectory("..");
}
int FTPClient::SetUTF8()
{
    fout << ">ftp opts utf8 on"  << "  ";//<< endl;
    string utf8_cmd = "opts utf8 on\r\n";
    send(this->ConnSocket, utf8_cmd.c_str(), utf8_cmd.size(),0);
    string servresponse = GetResponse(this->ConnSocket);
    //fout << servresponse << endl;
    if(CheckResponse(servresponse, 200) == 1)
        fout << "OK" << endl;
    else
    {
        fout << "Err" << endl;
    }
}
int FTPClient::CreatChildDir(string fatherDirPath, string childDirName)
{
    string childDirPath = fatherDirPath + "//" + childDirName;
    if(access(fatherDirPath.c_str(), 0) == 0)
    {
        if(access(childDirPath.c_str(), 0) == 0 || mkdir(childDirPath.c_str(), 0) == 0)
            return 1;   
    }
    return -1;
    
}
/*
int FTPClient::AutoDownloadDir(string localDir, string ftpDir)
{
    vector<string> vec_str = StringSplit(ftpDir, "/");
    string childDirName = vec_str[vec_str.size()-1];
    if(CreatChildDir(localDir,childDirName) == -1)
    {
        fout << "can not creat directory" << childDirName << " at localhost" << endl;
        return -1;
    }
    char *ptr_c = new char[ftpDir.size() + 1];
    memcpy(ptr_c, ftpDir.c_str(), ftpDir.size());
    FileNode *root = new FileNode(ptr_c);

    //FileIterator(ftpDir,root);

}
*/
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
int main()
{
    
    FTPClient ftp;
    ftp.LoginFTPServer("192.168.2.138", 21, "PC", "123456");
    //ftp.ChangeDirectory("GitHub/RSA");
    //ftp.ListDirectory();
    //ftp.DownloadFile("data.txt");
    //ftp.DownloadFile("111");
    ftp.SetUTF8();
    /*
    ftp.ChangeDirectory("ftp/测试");
    string str_info = ftp.ListDirectory();
    fout << str_info << endl;
    */
    string localDir = "/root/Documents/test_ftp";
    string tar_directory = "/GitHub/My-Notes";//;
    char *ptr_c = new char[tar_directory.size() + 1];
    memcpy(ptr_c, tar_directory.c_str(), tar_directory.size());
    FileNode *root = new FileNode(ptr_c);
    //root->SetName(tar_directory);
    /*
    char *ptr_c = new char[tar_directory.size()];
    memcpy(ptr_c, tar_directory.c_str(), tar_directory.size());
    root->name = ptr_c;
    */
    ftp.FileIterator(localDir, tar_directory);
    return 0;
}
