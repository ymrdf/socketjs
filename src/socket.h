
#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(_WIN32)
#  define SOCKET_OS_WIN
#endif

#include <nan.h>
#include <napi.h>


#include <sys/types.h>

#ifdef SOCKET_OS_WIN
# include <mstcpip.h> /* for SIO_RCVALL */
# include <winsock2.h>
# pragma comment (lib, "ws2_32.lib")  //加载 ws2_32.dll
#else
# include <unistd.h>
# include <netinet/in.h>
# include <net/ethernet.h> /* 链路层（L2）协议 */
# include <sys/socket.h>
#endif


#ifdef SOCKET_OS_WIN
#define SOCKETCLOSE closesocket
#endif

#ifndef SOCKETCLOSE
#define SOCKETCLOSE close
#endif

#ifdef SOCKET_OS_WIN
typedef SOCKET SOCKET_T;
#       if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#               define SIZEOF_SOCKET_T 8
#       else
#               define SIZEOF_SOCKET_T 4
#       endif
#else
typedef int SOCKET_T;
#       define SIZEOF_SOCKET_T SIZEOF_INT
#endif

#define NODE_SOCKET_SET_MACRO(env,m,v) m.Set(Napi::String::New(env, #v), Napi::Number::New(env, v));
#define NODE_SOCKET_SET_CONSTANT_ENV(env,m,p,v) m.Set(Napi::String::New(env, p), Napi::Number::New(env, v));