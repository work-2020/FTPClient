#include <winsock2.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib");
#pragma comment(lib, "wsock32.lib");
using namespace std;
int main()
{
    SOCKET soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    std::cout << "hello world" << std::endl;
    return 0;
}