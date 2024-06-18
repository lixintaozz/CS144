#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  TCPSocket tcpsock{};  //客户端创建套接字对象

  //使用服务器的域名以及提供的服务类型构建Address对象，这里也可以直接使用ip地址和协议名称
  Address sev_address("cs144.keithw.org", "http");


  //服务端的话需要使用bind()来将socket绑定到特定的ip地址（一般是本机）和端口
  //而客户端无需bind()，直接connect()服务器的ip地址和端口即可
  //客户端通过套接字连接到服务器
  tcpsock.connect(sev_address);

  //发送字符或者字符串数据不需要转换为大端存储方式
  string loads = string("GET ") + path + string(" HTTP/1.1\r\nHost: ") + host + string("\r\nConnection: close\r\n\r\n");
  tcpsock.write(loads);   //发送http GET 请求给服务器
  tcpsock.shutdown(SHUT_WR);   //关闭端口的写功能，避免服务器处于等待状态
  string sev_reply;
  cout << "Get web successfully!" << endl << "Response from server is: " << endl << endl ;

  //接收来自服务器的响应信息，由于网络不稳定，所以需要使用while循环来持续读取
  while(!tcpsock.eof()){
    tcpsock.read(sev_reply);
    cout << sev_reply ; 
  } 
  tcpsock.shutdown(SHUT_RD);  //关闭端口的读功能
  tcpsock.close();   //断开端口连接
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
