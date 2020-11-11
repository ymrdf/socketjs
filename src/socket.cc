// socket.cc
#include "socket.h"

namespace node_socket {

// using v8::Exception;
// using Nan::FunctionCallbackInfo;
// using v8::Isolate;
// using v8::Local;
// using v8::Object;
// using v8::Array;
// using v8::Number;
// using v8::String;
// using v8::Value;
// using v8::Context;
// using v8::MaybeLocal;
// using v8::Integer;
// using v8::Uint32;

sockaddr_in value_to_addr(Napi::Value orgv){
    Napi::Object addr  = orgv.As<Napi::Object>();

    int sin_family = addr.Get("sin_family").As<Napi::Number>().Int32Value();
    int sin_port = addr.Get("sin_port").As<Napi::Number>().Int32Value();
    unsigned int sin_addr = addr.Get("s_addr").As<Napi::Number>().Uint32Value();
    
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));  //每个字节都用0填充
    sockaddr.sin_family = sin_family;
    sockaddr.sin_port = htons(sin_port);
    sockaddr.sin_addr.s_addr = htonl(sin_addr);

    return sockaddr;
}

Napi::Object addr_to_value(Napi::Env env, sockaddr_in sockaddr){
    Napi::Object result = Napi::Object::New(env);
    result.Set(Napi::String::New(env, "sin_family"),Napi::Number::New(env, sockaddr.sin_family));
    result.Set(Napi::String::New(env, "sin_port"),Napi::Number::New(env, ntohs(sockaddr.sin_port)));
    result.Set(Napi::String::New(env, "s_addr"),Napi::Number::New(env, ntohl(sockaddr.sin_addr.s_addr)));
    return result;
}

void error_handle(Napi::Env env,const char *e){
    perror(e);
    Napi::TypeError::New(env, e).ThrowAsJavaScriptException();
}

void judge_fn_result(Napi::Env env, int r, const char *e){
#ifdef SOCKET_OS_WIN
    if(r == SOCKET_ERROR){
        error_handle(env, e);
    }
#else
    if(r < 0){
       error_handle(env, e);
    }
#endif
}


class AcceptAsyncWorker : public Napi::AsyncWorker {
public:
    AcceptAsyncWorker(Napi::Function& callback, int n)
            : AsyncWorker(callback), n_(n) {}

    void Execute() override {
        // 定义sockaddr_in
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        
        int res = accept(n_,(struct sockaddr *)&client_addr, &length);

        judge_fn_result(Env(), res, "bind error");
        result = res;
    }

    void OnOK() override {
        Callback().Call({Env().Null(), Napi::Number::New(Env(), result)});
    }

private:
    int n_;
    unsigned long long result;
};

class RecvAsyncWorker : public Napi::AsyncWorker {
public:
    RecvAsyncWorker(Napi::Function& callback, int n, char* buffer, int buflen, int flag)
        : AsyncWorker(callback) {
            n_ = n;
            buffer_ = buffer;
            buflen_ = buflen;
            flag_ = flag;
        }

    void Execute() override {
        long res = recv(n_, buffer_, buflen_, flag_);
        result = res;
    }

    void OnOK() override {
        Callback().Call({Env().Null(), Napi::Number::New(Env(), result)});
    }

private:
    int n_;
    char * buffer_;
    int buflen_;
    int flag_;
    unsigned long long result;
};

class RecvfromAsyncWorker : public Napi::AsyncWorker {
public:
    RecvfromAsyncWorker(Napi::Function& callback, int n, char* buffer, int buflen, int flag)
        : AsyncWorker(callback) {
            n_ = n;
            buffer_ = buffer;
            buflen_ = buflen;
            flag_ = flag;
        }

    void Execute() override {
        struct sockaddr_in from_addr;

        socklen_t length = sizeof(from_addr);
    
        long len = recvfrom(n_, buffer_, buflen_, flag_, (struct sockaddr *)&from_addr, &length);

        from_addr_ = from_addr;
        len_ = len;
    }

    void OnOK() override {

        Napi::Object res = Napi::Object::New(Env());
        
        res.Set(Napi::String::New(Env(), "length"), Napi::Number::New(Env(), len_));
        res.Set(Napi::String::New(Env(), "fromAddr"), addr_to_value(Env(), from_addr_ ));

        Callback().Call({Env().Null(),  res});
    }

private:
    int n_;
    int buflen_;
    int flag_;
    struct sockaddr_in from_addr_;
    unsigned long long len_;
    char *buffer_;
};


Napi::Value Socket(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 3) {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsNumber() || !args[2].IsNumber()) {
        Napi::TypeError::New(env, "Wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
  
    int fd = socket(args[0].As<Napi::Number>().Int32Value(), args[1].As<Napi::Number>().Int32Value(), htons(args[2].As<Napi::Number>().Int32Value()));

    if(fd < 0) {
        perror("socket error");
        Napi::TypeError::New(env, "socket error").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::Number::New(env, fd);
}


Napi::Value Bind(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 2) {
        Napi::TypeError::New(env, "Expected two arguments")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsObject()) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer, object")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    struct sockaddr_in server_sockaddr = value_to_addr(args[1]);

    int id = args[0].As<Napi::Number>().Int32Value();

    int result = bind(id,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr));

    judge_fn_result(env, result, "bind error");
    return Napi::Number::New(env, result);
}


Napi::Value Listen(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 2) {
        Napi::TypeError::New(env, "Expected two arguments")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsNumber() ) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer, integer").ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int backlog = args[1].As<Napi::Number>().Int32Value();

    int result = listen(fd, backlog);

    judge_fn_result(env, result, "listen error");
    return Napi::Number::New(env, result);
}

Napi::Value Accept(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();


    if (args.Length() < 1) {
        Napi::TypeError::New(env, "Expected just one arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber()) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    struct sockaddr_in client_addr;


    if(args.Length() > 1){
        client_addr =  value_to_addr(args[1]);
    }

    int id = args[0].As<Napi::Number>().Int32Value();

    socklen_t length = sizeof(client_addr);

    int result = accept(id,(struct sockaddr *)&client_addr, &length);

    judge_fn_result(env, result, "bind error");

    Napi::Object res = Napi::Object::New(env);
    
    res.Set(Napi::String::New(env, "fd"), Napi::Number::New(env, result));
    res.Set(Napi::String::New(env, "addr"), addr_to_value(env, client_addr ));

    return res;
}


Napi::Value Recv(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();
    

    if (args.Length() < 4) {
        Napi::TypeError::New(env, "Expected four arguments")
                    .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsArrayBuffer() || !args[2].IsNumber() || !args[3].IsNumber() ) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer, uint8array, integer, integer")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int buflen = args[2].As<Napi::Number>().Int32Value();
    int flag = args[3].As<Napi::Number>().Int32Value();

    Napi::ArrayBuffer buf = args[1].As<Napi::ArrayBuffer>();
    char* buffer = reinterpret_cast<char*>(buf.Data());
    long len = recv(fd, buffer, buflen, flag);

    return Napi::Number::New(env, len);
}

Napi::Value AsyncAccept(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();


    if (args.Length() < 2) {
        Napi::TypeError::New(env, "Expected two arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsFunction()) {
        Napi::TypeError::New(env, "Arguments wrong")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Function cb = args[1].As<Napi::Function>();

    int id = args[0].As<Napi::Number>().Int32Value();

    AcceptAsyncWorker* worker = new AcceptAsyncWorker(cb, id);
    worker->Queue();
    return Napi::String::New(env, "start calc fibonacci");
}

Napi::Value AsyncRecv(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 5) {
        Napi::TypeError::New(env, "Expected four arguments")
                    .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsArrayBuffer() || !args[2].IsNumber() || !args[3].IsNumber() || !args[4].IsFunction() ) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer, array buffer, integer, integer")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int buflen = args[2].As<Napi::Number>().Int32Value();
    int flag = args[3].As<Napi::Number>().Int32Value();

    Napi::ArrayBuffer buf = args[1].As<Napi::ArrayBuffer>();
    char* buffer = reinterpret_cast<char*>(buf.Data());
    Napi::Function cb = args[4].As<Napi::Function>();


    RecvAsyncWorker* worker = new RecvAsyncWorker(cb, fd, buffer, buflen, flag);
    worker->Queue();
    return Napi::String::New(env, "start calc fibonacci");
}


Napi::Value Recvfrom(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 4) {
        Napi::TypeError::New(env, "Expected four arguments")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsArrayBuffer() || !args[2].IsNumber() || !args[3].IsNumber() ) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer, ArrayBuffer, integer, integer, address object")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int buflen = args[2].As<Napi::Number>().Int32Value();
    int flag = args[3].As<Napi::Number>().Int32Value();


    Napi::ArrayBuffer buf = args[1].As<Napi::ArrayBuffer>();
    char* buffer = reinterpret_cast<char*>(buf.Data());


    struct sockaddr_in from_addr;

    socklen_t length = sizeof(from_addr);
 
    long len = recvfrom(fd, buffer, buflen, flag, (struct sockaddr *)&from_addr, &length);

    Napi::Object res = Napi::Object::New(env);
    res.Set(Napi::String::New(env, "length"), Napi::Number::New(env, len));
    res.Set(Napi::String::New(env, "buffer"), buf);
    res.Set(Napi::String::New(env, "fromAddr"), addr_to_value(env, from_addr ));

    return res;
}

Napi::Value AsyncRecvfrom(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 5) {
        Napi::TypeError::New(env, "Expected three arguments")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsArrayBuffer() || !args[2].IsNumber() || !args[3].IsNumber() || !args[4].IsFunction()) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer, integer, integer")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int buflen = args[2].As<Napi::Number>().Int32Value();
    int flag = args[3].As<Napi::Number>().Int32Value();

    Napi::ArrayBuffer buf = args[1].As<Napi::ArrayBuffer>();
    char* buffer = reinterpret_cast<char*>(buf.Data());
    Napi::Function cb = args[4].As<Napi::Function>();

    RecvfromAsyncWorker* worker = new RecvfromAsyncWorker(cb, fd, buffer, buflen, flag);
    worker->Queue();
    return Napi::Number::New(env, 1);
}


Napi::Value Sendto(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 5) {
            Napi::TypeError::New(env, "Expected five arguments")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsArrayBuffer() || !args[2].IsNumber() || !args[3].IsNumber()
        || !args[4].IsObject() ) {
            Napi::TypeError::New(env, "Supplied arguments should be: integer, uint8array, integer, integer, address object")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int buflen = args[2].As<Napi::Number>().Int32Value();
    int flag = args[3].As<Napi::Number>().Int32Value();

    Napi::ArrayBuffer buf = args[1].As<Napi::ArrayBuffer>();
    char* buffer = reinterpret_cast<char*>(buf.Data());

    // 定义sockaddr_in
    struct sockaddr_in socket_addr =  value_to_addr(args[4]);

    int len = sendto(fd, buffer, buflen, flag, (struct sockaddr *)&socket_addr, sizeof(socket_addr));

    judge_fn_result(env, len, "bind error");

    return Napi::Number::New(env, len);
}


Napi::Value Send(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 4) {
                Napi::TypeError::New(env, "Expected four arguments")
                    .ThrowAsJavaScriptException();
            return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsArrayBuffer() || !args[2].IsNumber() || !args[3].IsNumber() ) {
                Napi::TypeError::New(env, "Supplied arguments should be: integer, uint8array, integer, integer")
                    .ThrowAsJavaScriptException();
            return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int buflen = args[2].As<Napi::Number>().Int32Value();
    int flag = args[3].As<Napi::Number>().Int32Value();

    Napi::ArrayBuffer buf = args[1].As<Napi::ArrayBuffer>();

    char* buffer = reinterpret_cast<char*>(buf.Data());

    fwrite(buffer, buflen, 1, stdout);
    int result = send(fd, buffer, buflen, flag);

    judge_fn_result(env, result, "send error");
    return Napi::Number::New(env, result);
}

Napi::Value Connect(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 2) {
        Napi::TypeError::New(env, "Expected two arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsObject()) {
                    Napi::TypeError::New(env, "Supplied arguments should be: integer, address object")
                    .ThrowAsJavaScriptException();
            return env.Null();
    }

    sockaddr_in server_sockaddr  =  value_to_addr(args[1]);

    int id = args[0].As<Napi::Number>().Int32Value();

    int result = connect(id,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr));

    judge_fn_result(env, result, "connect error");
    
    return Napi::Number::New(env, result);
}


Napi::Value Close(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 1) {
        Napi::TypeError::New(env, "Expected just one argument")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    // 检查参数的类型。
    if (!args[0].IsNumber()) {
        Napi::TypeError::New(env, "Supplied argument should be: integer ").ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();

    int result = SOCKETCLOSE(fd);

    judge_fn_result(env, result, "close error");

    return Napi::Number::New(env, result);
}


Napi::Value Shutdown(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 2) {
        Napi::TypeError::New(env, "Expected just two argument")
                .ThrowAsJavaScriptException();
        return env.Null();
    }

    // 检查参数的类型。
    if (!args[0].IsNumber() || !args[1].IsNumber()) {
        Napi::TypeError::New(env, "Supplied argument should be: integer, integer ").ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int type = args[1].As<Napi::Number>().Int32Value();

    int result = shutdown(fd, type);

    judge_fn_result(env, result, "shutdown error");

    return Napi::Number::New(env, result);
}


Napi::Value Setsockopt(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 3) {
        Napi::TypeError::New(env, "Expected tree arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsNumber() || (!args[2].IsNumber() && !args[2].IsArrayBuffer()) ) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer, integer, integer/buffer")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
  
    int fd = args[0].As<Napi::Number>().Int32Value();
    int level = args[1].As<Napi::Number>().Int32Value();
    int optname = args[2].As<Napi::Number>().Int32Value();

    int result = -1;

    if(args[3].IsNumber()){
        int optval = args[3].As<Napi::Number>().Int32Value();
        result = setsockopt(fd, level, optname, (char *)& optval, sizeof(optval));
    }else{
        Napi::ArrayBuffer buf = args[3].As<Napi::ArrayBuffer>();
        char* buffer = reinterpret_cast<char*>(buf.Data());
        result = setsockopt(fd, level, optname, buffer, sizeof(buffer));
    }

    judge_fn_result(env, result, "setsockopt error");

    return Napi::Number::New(env, result);
}


Napi::Value Getsockopt(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 4) {
        Napi::TypeError::New(env, "Expected tree arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() || !args[1].IsNumber() || !args[2].IsNumber() || !args[3].IsNumber() ) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer, integer, integer, integer")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = args[0].As<Napi::Number>().Int32Value();
    int level = args[1].As<Napi::Number>().Int32Value();
    int optname = args[2].As<Napi::Number>().Int32Value();
    int len = args[3].As<Napi::Number>().Int32Value();

    socklen_t default_len = 0;
    socklen_t * optLen = &default_len;
    Napi::ArrayBuffer buf;

    if(len == 0){
        buf = Napi::ArrayBuffer::New(env, 1);
    }else{
        buf = Napi::ArrayBuffer::New(env, len);
    }

    char* buffer = reinterpret_cast<char*>(buf.Data());

    int result = getsockopt(fd, level, optname, (char *)& buffer, optLen);

    judge_fn_result(env, result, "getsockopt error");

    Napi::Object res = Napi::Object::New(env);
    
    res.Set(Napi::String::New(env, "result"), Napi::Number::New(env, result));
    res.Set(Napi::String::New(env, "buffer"), buf);
    res.Set(Napi::String::New(env, "bufferLen"), Napi::Number::New(env, * optLen) );

    return res;
}


Napi::Value Getsockname(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 1) {
        Napi::TypeError::New(env, "Expected one arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!args[0].IsNumber() ) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    struct sockaddr_in local_sockaddr;
    socklen_t len = sizeof(local_sockaddr);
    int id = args[0].As<Napi::Number>().Int32Value();

    int result = getsockname(id,(struct sockaddr *)&local_sockaddr, &len);

    Napi::Object res = Napi::Object::New(env);
    
    res.Set(Napi::String::New(env, "result"), Napi::Number::New(env, result));
    res.Set(Napi::String::New(env, "addr"), addr_to_value(env, local_sockaddr ));

    return res;
}

Napi::Value Getpeername(const Napi::CallbackInfo& args) {
    Napi::Env env = args.Env();

    if (args.Length() < 1) {
        Napi::TypeError::New(env, "Expected one arguments")
                    .ThrowAsJavaScriptException();
            return env.Null();
    }

    if (!args[0].IsNumber()) {
        Napi::TypeError::New(env, "Supplied arguments should be: integer")
                    .ThrowAsJavaScriptException();
            return env.Null();
    }

    struct sockaddr_in server_sockaddr;
    socklen_t len = sizeof(server_sockaddr);

    int id = args[0].As<Napi::Number>().Int32Value();

    int result = getpeername(id,(struct sockaddr *)&server_sockaddr, &len);

    Napi::Object res = Napi::Object::New(env);
    
    res.Set(Napi::String::New(env, "result"), Napi::Number::New(env, result));
    res.Set(Napi::String::New(env, "addr"), addr_to_value(env, server_sockaddr ));

    return res;
}


#ifdef SOCKET_OS_WIN
#define OS_INIT_DEFINED

/* Additional initialization and cleanup for Windows */

static void
os_cleanup(void)
{
    WSACleanup();
}

static int
os_init(void)
{
    WSADATA WSAData;
    int ret;
    ret = WSAStartup(0x0101, &WSAData);
    switch (ret) {
    case 0:     /* No error */
        // Py_AtExit(os_cleanup);
        return 1; /* Success */
    case WSASYSNOTREADY:
         perror("WSAStartup failed: network not ready");
        break;
    case WSAVERNOTSUPPORTED:
    case WSAEINVAL:
        perror("WSAStartup failed: requested version not supported");
        break;
    default:
        perror("WSAStartup failed: error code %d");
        break;
    }
    return 0; /* Failure */
}

#endif /* MS_WINDOWS */



#ifndef OS_INIT_DEFINED
static int
os_init(void)
{
    return 1; /* Success */
}
#endif


Napi::Object Init(Napi::Env env, Napi::Object exports) {
    os_init();

    exports.Set(Napi::String::New(env, "_socket"),
            Napi::Function::New(env, Socket));
    
    exports.Set(Napi::String::New(env, "_bind"),
            Napi::Function::New(env, Bind));

    exports.Set(Napi::String::New(env, "_close"),
            Napi::Function::New(env, Close));

    exports.Set(Napi::String::New(env, "_shutdown"),
            Napi::Function::New(env, Shutdown));

    exports.Set(Napi::String::New(env, "_listen"),
            Napi::Function::New(env, Listen));
    
    exports.Set(Napi::String::New(env, "_accept"),
            Napi::Function::New(env, Accept));
              
    exports.Set(Napi::String::New(env, "_asyncAccept"),
            Napi::Function::New(env, AsyncAccept));

    exports.Set(Napi::String::New(env, "_recv"),
            Napi::Function::New(env, Recv));
              
    exports.Set(Napi::String::New(env, "_asyncRecvfrom"),
            Napi::Function::New(env, AsyncRecvfrom));

    exports.Set(Napi::String::New(env, "_recvfrom"),
            Napi::Function::New(env, Recvfrom));

    exports.Set(Napi::String::New(env, "_sendto"),
            Napi::Function::New(env, Sendto));

    exports.Set(Napi::String::New(env, "_send"),
            Napi::Function::New(env, Send));

    exports.Set(Napi::String::New(env, "_connect"),
            Napi::Function::New(env, Connect));

    exports.Set(Napi::String::New(env, "_asyncRecv"),
            Napi::Function::New(env, AsyncRecv));

    exports.Set(Napi::String::New(env, "_getsockname"),
            Napi::Function::New(env, Getsockname));

    exports.Set(Napi::String::New(env, "_getpeername"),
            Napi::Function::New(env, Getpeername));

    exports.Set(Napi::String::New(env, "_setsockopt"),
            Napi::Function::New(env, Setsockopt));

    exports.Set(Napi::String::New(env, "_getsockopt"),
            Napi::Function::New(env, Getsockopt));


#define NODE_SOCKET_SET_CONSTANT(m,p,v)  NODE_SOCKET_SET_CONSTANT_ENV(env, m, p, v);

      /* Address families (we only support AF_INET and AF_UNIX) */
#ifdef AF_UNSPEC
    NODE_SOCKET_SET_MACRO(env, exports, AF_UNSPEC);
#endif
    NODE_SOCKET_SET_MACRO(env, exports, AF_INET);
#if defined(AF_UNIX)
    NODE_SOCKET_SET_MACRO(env, exports, AF_UNIX);
#endif /* AF_UNIX */
#ifdef AF_AX25
    /* Amateur Radio AX.25 */
    NODE_SOCKET_SET_MACRO(env, exports, AF_AX25);
#endif
#ifdef AF_IPX
    NODE_SOCKET_SET_MACRO(env, exports, AF_IPX); /* Novell IPX */
#endif
#ifdef AF_APPLETALK
    /* Appletalk DDP */
    NODE_SOCKET_SET_MACRO(env, exports, AF_APPLETALK);
#endif
#ifdef AF_NETROM
    /* Amateur radio NetROM */
    NODE_SOCKET_SET_MACRO(env, exports, AF_NETROM);
#endif
#ifdef AF_BRIDGE
    /* Multiprotocol bridge */
    NODE_SOCKET_SET_MACRO(env, exports, AF_BRIDGE);
#endif
#ifdef AF_ATMPVC
    /* ATM PVCs */
    NODE_SOCKET_SET_MACRO(env, exports, AF_ATMPVC);
#endif
#ifdef AF_AAL5
    /* Reserved for Werner's ATM */
    NODE_SOCKET_SET_MACRO(env, exports, AF_AAL5);
#endif
#ifdef HAVE_SOCKADDR_ALG
    NODE_SOCKET_SET_MACRO(env, exports, AF_ALG); /* Linux crypto */
#endif
#ifdef AF_X25
    /* Reserved for X.25 project */
    NODE_SOCKET_SET_MACRO(env, exports, AF_X25);
#endif
#ifdef AF_INET6
    NODE_SOCKET_SET_MACRO(env, exports, AF_INET6); /* IP version 6 */
#endif
#ifdef AF_ROSE
    /* Amateur Radio X.25 PLP */
    NODE_SOCKET_SET_MACRO(env, exports, AF_ROSE);
#endif
#ifdef AF_DECnet
    /* Reserved for DECnet project */
    NODE_SOCKET_SET_MACRO(env, exports, AF_DECnet);
#endif
#ifdef AF_NETBEUI
    /* Reserved for 802.2LLC project */
    NODE_SOCKET_SET_MACRO(env, exports, AF_NETBEUI);
#endif
#ifdef AF_SECURITY
    /* Security callback pseudo AF */
    NODE_SOCKET_SET_MACRO(env, exports, AF_SECURITY);
#endif
#ifdef AF_KEY
    /* PF_KEY key management API */
    NODE_SOCKET_SET_MACRO(env, exports, AF_KEY);
#endif
#ifdef AF_NETLINK
    /*  */
    NODE_SOCKET_SET_MACRO(env, exports, AF_NETLINK);
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_ROUTE);
#ifdef NETLINK_SKIP
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_SKIP);
#endif
#ifdef NETLINK_W1
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_W1);
#endif
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_USERSOCK);
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_FIREWALL);
#ifdef NETLINK_TCPDIAG
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_TCPDIAG);
#endif
#ifdef NETLINK_NFLOG
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_NFLOG);
#endif
#ifdef NETLINK_XFRM
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_XFRM);
#endif
#ifdef NETLINK_ARPD
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_ARPD);
#endif
#ifdef NETLINK_ROUTE6
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_ROUTE6);
#endif
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_IP6_FW);
#ifdef NETLINK_DNRTMSG
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_DNRTMSG);
#endif
#ifdef NETLINK_TAPBASE
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_TAPBASE);
#endif
#ifdef NETLINK_CRYPTO
    NODE_SOCKET_SET_MACRO(env, exports, NETLINK_CRYPTO);
#endif
#endif /* AF_NETLINK */

#ifdef AF_QIPCRTR
    /* Qualcomm IPCROUTER */
    NODE_SOCKET_SET_MACRO(env, exports, AF_QIPCRTR);
#endif

#ifdef AF_VSOCK
    NODE_SOCKET_SET_CONSTANT(exports, "AF_VSOCK", AF_VSOCK);
    NODE_SOCKET_SET_CONSTANT(exports, "SO_VM_SOCKETS_BUFFER_SIZE", 0);
    NODE_SOCKET_SET_CONSTANT(exports, "SO_VM_SOCKETS_BUFFER_MIN_SIZE", 1);
    NODE_SOCKET_SET_CONSTANT(exports, "SO_VM_SOCKETS_BUFFER_MAX_SIZE", 2);
    NODE_SOCKET_SET_CONSTANT(exports, "VMADDR_CID_ANY", 0xffffffff);
    NODE_SOCKET_SET_CONSTANT(exports, "VMADDR_PORT_ANY", 0xffffffff);
    NODE_SOCKET_SET_CONSTANT(exports, "VMADDR_CID_HOST", 2);
    NODE_SOCKET_SET_CONSTANT(exports, "VM_SOCKETS_INVALID_VERSION", 0xffffffff);
    NODE_SOCKET_SET_CONSTANT(exports, "IOCTL_VM_SOCKETS_GET_LOCAL_CID",  _IO(7, 0xb9));
#endif

#ifdef AF_ROUTE
    /* Alias to emulate 4.4BSD */
    NODE_SOCKET_SET_MACRO(env, exports, AF_ROUTE);
#endif
#ifdef AF_LINK
    NODE_SOCKET_SET_MACRO(env, exports, AF_LINK);
#endif
#ifdef AF_ASH
    /* Ash */
    NODE_SOCKET_SET_MACRO(env, exports, AF_ASH);
#endif
#ifdef AF_ECONET
    /* Acorn Econet */
    NODE_SOCKET_SET_MACRO(env, exports, AF_ECONET);
#endif
#ifdef AF_ATMSVC
    /* ATM SVCs */
    NODE_SOCKET_SET_MACRO(env, exports, AF_ATMSVC);
#endif
#ifdef AF_SNA
    /* Linux SNA Project (nutters!) */
    NODE_SOCKET_SET_MACRO(env, exports, AF_SNA);
#endif
#ifdef AF_IRDA
    /* IRDA sockets */
    NODE_SOCKET_SET_MACRO(env, exports, AF_IRDA);
#endif
#ifdef AF_PPPOX
    /* PPPoX sockets */
    NODE_SOCKET_SET_MACRO(env, exports, AF_PPPOX);
#endif
#ifdef AF_WANPIPE
    /* Wanpipe API Sockets */
    NODE_SOCKET_SET_MACRO(env, exports, AF_WANPIPE);
#endif
#ifdef AF_LLC
    /* Linux LLC */
    NODE_SOCKET_SET_MACRO(env, exports, AF_LLC);
#endif

#ifdef USE_BLUETOOTH
    NODE_SOCKET_SET_MACRO(env, exports, AF_BLUETOOTH);
    NODE_SOCKET_SET_MACRO(env, exports, BTPROTO_L2CAP);
    NODE_SOCKET_SET_MACRO(env, exports, BTPROTO_HCI);
    NODE_SOCKET_SET_MACRO(env, exports, SOL_HCI);
#if !defined(__NetBSD__) && !defined(__DragonFly__)
    NODE_SOCKET_SET_MACRO(env, exports, HCI_FILTER);
#endif
#if !defined(__FreeBSD__)
#if !defined(__NetBSD__) && !defined(__DragonFly__)
    NODE_SOCKET_SET_MACRO(env, exports, HCI_TIME_STAMP);
#endif
    NODE_SOCKET_SET_MACRO(env, exports, HCI_DATA_DIR);
    NODE_SOCKET_SET_MACRO(env, exports, BTPROTO_SCO);
#endif
    NODE_SOCKET_SET_MACRO(env, exports, BTPROTO_RFCOMM);
    // PyModule_AddStringConstant(m, "BDADDR_ANY", "00:00:00:00:00:00");
    // PyModule_AddStringConstant(m, "BDADDR_LOCAL", "00:00:00:FF:FF:FF");
#endif

#ifdef AF_CAN
    /* Controller Area Network */
    NODE_SOCKET_SET_MACRO(env, exports, AF_CAN);
#endif
#ifdef PF_CAN
    /* Controller Area Network */
    NODE_SOCKET_SET_MACRO(env, exports, PF_CAN);
#endif

/* Reliable Datagram Sockets */
#ifdef AF_RDS
    NODE_SOCKET_SET_MACRO(env, exports, AF_RDS);
#endif
#ifdef PF_RDS
    NODE_SOCKET_SET_MACRO(env, exports, PF_RDS);
#endif

/* Kernel event messages */
#ifdef PF_SYSTEM
    NODE_SOCKET_SET_MACRO(env, exports, PF_SYSTEM);
#endif
#ifdef AF_SYSTEM
    NODE_SOCKET_SET_MACRO(env, exports, AF_SYSTEM);
#endif

#ifdef AF_PACKET
    NODE_SOCKET_SET_MACRO(env, exports, AF_PACKET);
#endif
#ifdef PF_PACKET
    NODE_SOCKET_SET_MACRO(env, exports, PF_PACKET);
#endif
#ifdef PACKET_HOST
    NODE_SOCKET_SET_MACRO(env, exports, PACKET_HOST);
#endif
#ifdef PACKET_BROADCAST
    NODE_SOCKET_SET_MACRO(env, exports, PACKET_BROADCAST);
#endif
#ifdef PACKET_MULTICAST
    NODE_SOCKET_SET_MACRO(env, exports, PACKET_MULTICAST);
#endif
#ifdef PACKET_OTHERHOST
    NODE_SOCKET_SET_MACRO(env, exports, PACKET_OTHERHOST);
#endif
#ifdef PACKET_OUTGOING
    NODE_SOCKET_SET_MACRO(env, exports, PACKET_OUTGOING);
#endif
#ifdef PACKET_LOOPBACK
    NODE_SOCKET_SET_MACRO(env, exports, PACKET_LOOPBACK);
#endif
#ifdef PACKET_FASTROUTE
    NODE_SOCKET_SET_MACRO(env, exports, PACKET_FASTROUTE);
#endif

#ifdef HAVE_LINUX_TIPC_H
    NODE_SOCKET_SET_MACRO(env, exports, AF_TIPC);

    /* for addresses */
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_ADDR_NAMESEQ);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_ADDR_NAME);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_ADDR_ID);

    NODE_SOCKET_SET_MACRO(env, exports, TIPC_ZONE_SCOPE);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_CLUSTER_SCOPE);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_NODE_SCOPE);

    /* for setsockopt() */
    NODE_SOCKET_SET_MACRO(env, exports, SOL_TIPC);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_IMPORTANCE);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_SRC_DROPPABLE);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_DEST_DROPPABLE);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_CONN_TIMEOUT);

    NODE_SOCKET_SET_MACRO(env, exports, TIPC_LOW_IMPORTANCE);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_MEDIUM_IMPORTANCE);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_HIGH_IMPORTANCE);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_CRITICAL_IMPORTANCE);

    /* for subscriptions */
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_SUB_PORTS);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_SUB_SERVICE);
#ifdef TIPC_SUB_CANCEL
    /* doesn't seem to be available everywhere */
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_SUB_CANCEL);
#endif
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_WAIT_FOREVER);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_PUBLISHED);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_WITHDRAWN);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_SUBSCR_TIMEOUT);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_CFG_SRV);
    NODE_SOCKET_SET_MACRO(env, exports, TIPC_TOP_SRV);
#endif

#ifdef HAVE_SOCKADDR_ALG
    /* Socket options */
    NODE_SOCKET_SET_MACRO(env, exports, ALG_SET_KEY);
    NODE_SOCKET_SET_MACRO(env, exports, ALG_SET_IV);
    NODE_SOCKET_SET_MACRO(env, exports, ALG_SET_OP);
    NODE_SOCKET_SET_MACRO(env, exports, ALG_SET_AEAD_ASSOCLEN);
    NODE_SOCKET_SET_MACRO(env, exports, ALG_SET_AEAD_AUTHSIZE);
    NODE_SOCKET_SET_MACRO(env, exports, ALG_SET_PUBKEY);

    /* Operations */
    NODE_SOCKET_SET_MACRO(env, exports, ALG_OP_DECRYPT);
    NODE_SOCKET_SET_MACRO(env, exports, ALG_OP_ENCRYPT);
    NODE_SOCKET_SET_MACRO(env, exports, ALG_OP_SIGN);
    NODE_SOCKET_SET_MACRO(env, exports, ALG_OP_VERIFY);
#endif

    /* Socket types */
    NODE_SOCKET_SET_MACRO(env, exports, SOCK_STREAM);
    NODE_SOCKET_SET_MACRO(env, exports, SOCK_DGRAM);
/* We have incomplete socket support. */
#ifdef SOCK_RAW
    /* SOCK_RAW is marked as optional in the POSIX specification */
    NODE_SOCKET_SET_MACRO(env, exports, SOCK_RAW);
#endif
    NODE_SOCKET_SET_MACRO(env, exports, SOCK_SEQPACKET);
#if defined(SOCK_RDM)
    NODE_SOCKET_SET_MACRO(env, exports, SOCK_RDM);
#endif
#ifdef SOCK_CLOEXEC
    NODE_SOCKET_SET_MACRO(env, exports, SOCK_CLOEXEC);
#endif
#ifdef SOCK_NONBLOCK
    NODE_SOCKET_SET_MACRO(env, exports, SOCK_NONBLOCK);
#endif

#ifdef  SO_DEBUG
    NODE_SOCKET_SET_MACRO(env, exports, SO_DEBUG);
#endif
#ifdef  SO_ACCEPTCONN
    NODE_SOCKET_SET_MACRO(env, exports, SO_ACCEPTCONN);
#endif
#ifdef  SO_REUSEADDR
    NODE_SOCKET_SET_MACRO(env, exports, SO_REUSEADDR);
#endif
#ifdef SO_EXCLUSIVEADDRUSE
    NODE_SOCKET_SET_MACRO(env, exports, SO_EXCLUSIVEADDRUSE);
#endif

#ifdef  SO_KEEPALIVE
    NODE_SOCKET_SET_MACRO(env, exports, SO_KEEPALIVE);
#endif
#ifdef  SO_DONTROUTE
    NODE_SOCKET_SET_MACRO(env, exports, SO_DONTROUTE);
#endif
#ifdef  SO_BROADCAST
    NODE_SOCKET_SET_MACRO(env, exports, SO_BROADCAST);
#endif
#ifdef  SO_USELOOPBACK
    NODE_SOCKET_SET_MACRO(env, exports, SO_USELOOPBACK);
#endif
#ifdef  SO_LINGER
    NODE_SOCKET_SET_MACRO(env, exports, SO_LINGER);
#endif
#ifdef  SO_OOBINLINE
    NODE_SOCKET_SET_MACRO(env, exports, SO_OOBINLINE);
#endif
#ifndef __GNU__
#ifdef  SO_REUSEPORT
    NODE_SOCKET_SET_MACRO(env, exports, SO_REUSEPORT);
#endif
#endif
#ifdef  SO_SNDBUF
    NODE_SOCKET_SET_MACRO(env, exports, SO_SNDBUF);
#endif
#ifdef  SO_RCVBUF
    NODE_SOCKET_SET_MACRO(env, exports, SO_RCVBUF);
#endif
#ifdef  SO_SNDLOWAT
    NODE_SOCKET_SET_MACRO(env, exports, SO_SNDLOWAT);
#endif
#ifdef  SO_RCVLOWAT
    NODE_SOCKET_SET_MACRO(env, exports, SO_RCVLOWAT);
#endif
#ifdef  SO_SNDTIMEO
    NODE_SOCKET_SET_MACRO(env, exports, SO_SNDTIMEO);
#endif
#ifdef  SO_RCVTIMEO
    NODE_SOCKET_SET_MACRO(env, exports, SO_RCVTIMEO);
#endif
#ifdef  SO_ERROR
    NODE_SOCKET_SET_MACRO(env, exports, SO_ERROR);
#endif
#ifdef  SO_TYPE
    NODE_SOCKET_SET_MACRO(env, exports, SO_TYPE);
#endif
#ifdef  SO_SETFIB
    NODE_SOCKET_SET_MACRO(env, exports, SO_SETFIB);
#endif
#ifdef  SO_PASSCRED
    NODE_SOCKET_SET_MACRO(env, exports, SO_PASSCRED);
#endif
#ifdef  SO_PEERCRED
    NODE_SOCKET_SET_MACRO(env, exports, SO_PEERCRED);
#endif
#ifdef  LOCAL_PEERCRED
    NODE_SOCKET_SET_MACRO(env, exports, LOCAL_PEERCRED);
#endif
#ifdef  SO_PASSSEC
    NODE_SOCKET_SET_MACRO(env, exports, SO_PASSSEC);
#endif
#ifdef  SO_PEERSEC
    NODE_SOCKET_SET_MACRO(env, exports, SO_PEERSEC);
#endif
#ifdef  SO_BINDTODEVICE
    NODE_SOCKET_SET_MACRO(env, exports, SO_BINDTODEVICE);
#endif
#ifdef  SO_PRIORITY
    NODE_SOCKET_SET_MACRO(env, exports, SO_PRIORITY);
#endif
#ifdef  SO_MARK
    NODE_SOCKET_SET_MACRO(env, exports, SO_MARK);
#endif
#ifdef SO_DOMAIN
    NODE_SOCKET_SET_MACRO(env, exports, SO_DOMAIN);
#endif
#ifdef SO_PROTOCOL
    NODE_SOCKET_SET_MACRO(env, exports, SO_PROTOCOL);
#endif

    /* Maximum number of connections for "listen" */
#ifdef  SOMAXCONN
    NODE_SOCKET_SET_MACRO(env, exports, SOMAXCONN);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "SOMAXCONN", 5); /* Common value */
#endif

    /* Ancillary message types */
#ifdef  SCM_RIGHTS
    NODE_SOCKET_SET_MACRO(env, exports, SCM_RIGHTS);
#endif
#ifdef  SCM_CREDENTIALS
    NODE_SOCKET_SET_MACRO(env, exports, SCM_CREDENTIALS);
#endif
#ifdef  SCM_CREDS
    NODE_SOCKET_SET_MACRO(env, exports, SCM_CREDS);
#endif

    /* Flags for send, recv */
#ifdef  MSG_OOB
    NODE_SOCKET_SET_MACRO(env, exports, MSG_OOB);
#endif
#ifdef  MSG_PEEK
    NODE_SOCKET_SET_MACRO(env, exports, MSG_PEEK);
#endif
#ifdef  MSG_DONTROUTE
    NODE_SOCKET_SET_MACRO(env, exports, MSG_DONTROUTE);
#endif
#ifdef  MSG_DONTWAIT
    NODE_SOCKET_SET_MACRO(env, exports, MSG_DONTWAIT);
#endif
#ifdef  MSG_EOR
    NODE_SOCKET_SET_MACRO(env, exports, MSG_EOR);
#endif
#ifdef  MSG_TRUNC
    NODE_SOCKET_SET_MACRO(env, exports, MSG_TRUNC);
#endif
#ifdef  MSG_CTRUNC
    NODE_SOCKET_SET_MACRO(env, exports, MSG_CTRUNC);
#endif
#ifdef  MSG_WAITALL
    NODE_SOCKET_SET_MACRO(env, exports, MSG_WAITALL);
#endif
#ifdef  MSG_BTAG
    NODE_SOCKET_SET_MACRO(env, exports, MSG_BTAG);
#endif
#ifdef  MSG_ETAG
    NODE_SOCKET_SET_MACRO(env, exports, MSG_ETAG);
#endif
#ifdef  MSG_NOSIGNAL
    NODE_SOCKET_SET_MACRO(env, exports, MSG_NOSIGNAL);
#endif
#ifdef  MSG_NOTIFICATION
    NODE_SOCKET_SET_MACRO(env, exports, MSG_NOTIFICATION);
#endif
#ifdef  MSG_CMSG_CLOEXEC
    NODE_SOCKET_SET_MACRO(env, exports, MSG_CMSG_CLOEXEC);
#endif
#ifdef  MSG_ERRQUEUE
    NODE_SOCKET_SET_MACRO(env, exports, MSG_ERRQUEUE);
#endif
#ifdef  MSG_CONFIRM
    NODE_SOCKET_SET_MACRO(env, exports, MSG_CONFIRM);
#endif
#ifdef  MSG_MORE
    NODE_SOCKET_SET_MACRO(env, exports, MSG_MORE);
#endif
#ifdef  MSG_EOF
    NODE_SOCKET_SET_MACRO(env, exports, MSG_EOF);
#endif
#ifdef  MSG_BCAST
    NODE_SOCKET_SET_MACRO(env, exports, MSG_BCAST);
#endif
#ifdef  MSG_MCAST
    NODE_SOCKET_SET_MACRO(env, exports, MSG_MCAST);
#endif
#ifdef MSG_FASTOPEN
    NODE_SOCKET_SET_MACRO(env, exports, MSG_FASTOPEN);
#endif

    /* Protocol level and numbers, usable for [gs]etsockopt */
#ifdef  SOL_SOCKET
    NODE_SOCKET_SET_MACRO(env, exports, SOL_SOCKET);
#endif
#ifdef  SOL_IP
    NODE_SOCKET_SET_MACRO(env, exports, SOL_IP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "SOL_IP", 0);
#endif
#ifdef  SOL_IPX
    NODE_SOCKET_SET_MACRO(env, exports, SOL_IPX);
#endif
#ifdef  SOL_AX25
    NODE_SOCKET_SET_MACRO(env, exports, SOL_AX25);
#endif
#ifdef  SOL_ATALK
    NODE_SOCKET_SET_MACRO(env, exports, SOL_ATALK);
#endif
#ifdef  SOL_NETROM
    NODE_SOCKET_SET_MACRO(env, exports, SOL_NETROM);
#endif
#ifdef  SOL_ROSE
    NODE_SOCKET_SET_MACRO(env, exports, SOL_ROSE);
#endif
#ifdef  SOL_TCP
    NODE_SOCKET_SET_MACRO(env, exports, SOL_TCP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "SOL_TCP", 6);
#endif
#ifdef  SOL_UDP
    NODE_SOCKET_SET_MACRO(env, exports, SOL_UDP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "SOL_UDP", 17);
#endif
#ifdef SOL_CAN_BASE
    NODE_SOCKET_SET_MACRO(env, exports, SOL_CAN_BASE);
#endif
#ifdef SOL_CAN_RAW
    NODE_SOCKET_SET_MACRO(env, exports, SOL_CAN_RAW);
    NODE_SOCKET_SET_MACRO(env, exports, CAN_RAW);
#endif
#ifdef HAVE_LINUX_CAN_H
    NODE_SOCKET_SET_MACRO(env, exports, CAN_EFF_FLAG);
    NODE_SOCKET_SET_MACRO(env, exports, CAN_RTR_FLAG);
    NODE_SOCKET_SET_MACRO(env, exports, CAN_ERR_FLAG);

    NODE_SOCKET_SET_MACRO(env, exports, CAN_SFF_MASK);
    NODE_SOCKET_SET_MACRO(env, exports, CAN_EFF_MASK);
    NODE_SOCKET_SET_MACRO(env, exports, CAN_ERR_MASK);
#ifdef CAN_ISOTP
    NODE_SOCKET_SET_MACRO(env, exports, CAN_ISOTP);
#endif
#endif
#ifdef HAVE_LINUX_CAN_RAW_H
    NODE_SOCKET_SET_MACRO(env, exports, CAN_RAW_FILTER);
    NODE_SOCKET_SET_MACRO(env, exports, CAN_RAW_ERR_FILTER);
    NODE_SOCKET_SET_MACRO(env, exports, CAN_RAW_LOOPBACK);
    NODE_SOCKET_SET_MACRO(env, exports, CAN_RAW_RECV_OWN_MSGS);
#endif
#ifdef HAVE_LINUX_CAN_RAW_FD_FRAMES
    NODE_SOCKET_SET_MACRO(env, exports, CAN_RAW_FD_FRAMES);
#endif
#ifdef HAVE_LINUX_CAN_BCM_H
    NODE_SOCKET_SET_MACRO(env, exports, CAN_BCM);

    /* BCM opcodes */
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_SETUP", TX_SETUP);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_DELETE", TX_DELETE);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_READ", TX_READ);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_SEND", TX_SEND);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_SETUP", RX_SETUP);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_DELETE", RX_DELETE);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_READ", RX_READ);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_STATUS", TX_STATUS);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_EXPIRED", TX_EXPIRED);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_STATUS", RX_STATUS);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_TIMEOUT", RX_TIMEOUT);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_CHANGED", RX_CHANGED);

    /* BCM flags */
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_SETTIMER", SETTIMER);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_STARTTIMER", STARTTIMER);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_COUNTEVT", TX_COUNTEVT);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_ANNOUNCE", TX_ANNOUNCE);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_CP_CAN_ID", TX_CP_CAN_ID);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_FILTER_ID", RX_FILTER_ID);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_CHECK_DLC", RX_CHECK_DLC);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_NO_AUTOTIMER", RX_NO_AUTOTIMER);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_ANNOUNCE_RESUME", RX_ANNOUNCE_RESUME);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_TX_RESET_MULTI_IDX", TX_RESET_MULTI_IDX);
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_RX_RTR_FRAME", RX_RTR_FRAME);
#ifdef CAN_FD_FRAME
    /* CAN_FD_FRAME was only introduced in the 4.8.x kernel series */
    NODE_SOCKET_SET_CONSTANT(exports, "CAN_BCM_CAN_FD_FRAME", CAN_FD_FRAME);
#endif
#endif
#ifdef SOL_RDS
    NODE_SOCKET_SET_MACRO(env, exports, SOL_RDS);
#endif
#ifdef HAVE_SOCKADDR_ALG
    NODE_SOCKET_SET_MACRO(env, exports, SOL_ALG);
#endif
#ifdef RDS_CANCEL_SENT_TO
    NODE_SOCKET_SET_MACRO(env, exports, RDS_CANCEL_SENT_TO);
#endif
#ifdef RDS_GET_MR
    NODE_SOCKET_SET_MACRO(env, exports, RDS_GET_MR);
#endif
#ifdef RDS_FREE_MR
    NODE_SOCKET_SET_MACRO(env, exports, RDS_FREE_MR);
#endif
#ifdef RDS_RECVERR
    NODE_SOCKET_SET_MACRO(env, exports, RDS_RECVERR);
#endif
#ifdef RDS_CONG_MONITOR
    NODE_SOCKET_SET_MACRO(env, exports, RDS_CONG_MONITOR);
#endif
#ifdef RDS_GET_MR_FOR_DEST
    NODE_SOCKET_SET_MACRO(env, exports, RDS_GET_MR_FOR_DEST);
#endif
#ifdef  IPPROTO_IP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_IP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "IPPROTO_IP", 0);
#endif
#ifdef  IPPROTO_HOPOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_HOPOPTS);
#endif
#ifdef  IPPROTO_ICMP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_ICMP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "IPPROTO_ICMP", 1);
#endif
#ifdef  IPPROTO_IGMP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_IGMP);
#endif
#ifdef  IPPROTO_GGP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_GGP);
#endif
#ifdef  IPPROTO_IPV4
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_IPV4);
#endif
#ifdef  IPPROTO_IPV6
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_IPV6);
#endif
#ifdef  IPPROTO_IPIP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_IPIP);
#endif
#ifdef  IPPROTO_TCP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_TCP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "IPPROTO_TCP", 6);
#endif
#ifdef  IPPROTO_EGP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_EGP);
#endif
#ifdef  IPPROTO_PUP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_PUP);
#endif
#ifdef  IPPROTO_UDP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_UDP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "IPPROTO_UDP", 17);
#endif
#ifdef  IPPROTO_IDP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_IDP);
#endif
#ifdef  IPPROTO_HELLO
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_HELLO);
#endif
#ifdef  IPPROTO_ND
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_ND);
#endif
#ifdef  IPPROTO_TP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_TP);
#endif
#ifdef  IPPROTO_ROUTING
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_ROUTING);
#endif
#ifdef  IPPROTO_FRAGMENT
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_FRAGMENT);
#endif
#ifdef  IPPROTO_RSVP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_RSVP);
#endif
#ifdef  IPPROTO_GRE
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_GRE);
#endif
#ifdef  IPPROTO_ESP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_ESP);
#endif
#ifdef  IPPROTO_AH
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_AH);
#endif
#ifdef  IPPROTO_MOBILE
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_MOBILE);
#endif
#ifdef  IPPROTO_ICMPV6
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_ICMPV6);
#endif
#ifdef  IPPROTO_NONE
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_NONE);
#endif
#ifdef  IPPROTO_DSTOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_DSTOPTS);
#endif
#ifdef  IPPROTO_XTP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_XTP);
#endif
#ifdef  IPPROTO_EON
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_EON);
#endif
#ifdef  IPPROTO_PIM
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_PIM);
#endif
#ifdef  IPPROTO_IPCOMP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_IPCOMP);
#endif
#ifdef  IPPROTO_VRRP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_VRRP);
#endif
#ifdef  IPPROTO_SCTP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_SCTP);
#endif
#ifdef  IPPROTO_BIP
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_BIP);
#endif
/**/
#ifdef  IPPROTO_RAW
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_RAW);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "IPPROTO_RAW", 255);
#endif
#ifdef  IPPROTO_MAX
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_MAX);
#endif

#ifdef  MS_WINDOWS
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_ICLFXBM);
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_ST);
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_CBT);
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_IGP);
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_RDP);
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_PGM);
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_L2TP);
    NODE_SOCKET_SET_MACRO(env, exports, IPPROTO_SCTP);
#endif

#ifdef  SYSPROTO_CONTROL
    NODE_SOCKET_SET_MACRO(env, exports, SYSPROTO_CONTROL);
#endif

    /* Some port configuration */
#ifdef  IPPORT_RESERVED
    NODE_SOCKET_SET_MACRO(env, exports, IPPORT_RESERVED);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "IPPORT_RESERVED", 1024);
#endif
#ifdef  IPPORT_USERRESERVED
    NODE_SOCKET_SET_MACRO(env, exports, IPPORT_USERRESERVED);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "IPPORT_USERRESERVED", 5000);
#endif

    /* Some reserved IP v.4 addresses */
#ifdef  INADDR_ANY
    NODE_SOCKET_SET_MACRO(env, exports, INADDR_ANY);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "INADDR_ANY", 0x00000000);
#endif
#ifdef  INADDR_BROADCAST
    NODE_SOCKET_SET_MACRO(env, exports, INADDR_BROADCAST);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "INADDR_BROADCAST", 0xffffffff);
#endif
#ifdef  INADDR_LOOPBACK
    NODE_SOCKET_SET_MACRO(env, exports, INADDR_LOOPBACK);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "INADDR_LOOPBACK", 0x7F000001);
#endif
#ifdef  INADDR_UNSPEC_GROUP
    NODE_SOCKET_SET_MACRO(env, exports, INADDR_UNSPEC_GROUP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "INADDR_UNSPEC_GROUP", 0xe0000000);
#endif
#ifdef  INADDR_ALLHOSTS_GROUP
    NODE_SOCKET_SET_CONSTANT(exports, "INADDR_ALLHOSTS_GROUP",
                            INADDR_ALLHOSTS_GROUP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "INADDR_ALLHOSTS_GROUP", 0xe0000001);
#endif
#ifdef  INADDR_MAX_LOCAL_GROUP
    NODE_SOCKET_SET_MACRO(env, exports, INADDR_MAX_LOCAL_GROUP);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "INADDR_MAX_LOCAL_GROUP", 0xe00000ff);
#endif
#ifdef  INADDR_NONE
    NODE_SOCKET_SET_MACRO(env, exports, INADDR_NONE);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "INADDR_NONE", 0xffffffff);
#endif

    /* IPv4 [gs]etsockopt options */
#ifdef  IP_OPTIONS
    NODE_SOCKET_SET_MACRO(env, exports, IP_OPTIONS);
#endif
#ifdef  IP_HDRINCL
    NODE_SOCKET_SET_MACRO(env, exports, IP_HDRINCL);
#endif
#ifdef  IP_TOS
    NODE_SOCKET_SET_MACRO(env, exports, IP_TOS);
#endif
#ifdef  IP_TTL
    NODE_SOCKET_SET_MACRO(env, exports, IP_TTL);
#endif
#ifdef  IP_RECVOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IP_RECVOPTS);
#endif
#ifdef  IP_RECVRETOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IP_RECVRETOPTS);
#endif
#ifdef  IP_RECVDSTADDR
    NODE_SOCKET_SET_MACRO(env, exports, IP_RECVDSTADDR);
#endif
#ifdef  IP_RETOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IP_RETOPTS);
#endif
#ifdef  IP_MULTICAST_IF
    NODE_SOCKET_SET_MACRO(env, exports, IP_MULTICAST_IF);
#endif
#ifdef  IP_MULTICAST_TTL
    NODE_SOCKET_SET_MACRO(env, exports, IP_MULTICAST_TTL);
#endif
#ifdef  IP_MULTICAST_LOOP
    NODE_SOCKET_SET_MACRO(env, exports, IP_MULTICAST_LOOP);
#endif
#ifdef  IP_ADD_MEMBERSHIP
    NODE_SOCKET_SET_MACRO(env, exports, IP_ADD_MEMBERSHIP);
#endif
#ifdef  IP_DROP_MEMBERSHIP
    NODE_SOCKET_SET_MACRO(env, exports, IP_DROP_MEMBERSHIP);
#endif
#ifdef  IP_DEFAULT_MULTICAST_TTL
    NODE_SOCKET_SET_MACRO(env, exports, IP_DEFAULT_MULTICAST_TTL);
#endif
#ifdef  IP_DEFAULT_MULTICAST_LOOP
    NODE_SOCKET_SET_MACRO(env, exports, IP_DEFAULT_MULTICAST_LOOP);
#endif
#ifdef  IP_MAX_MEMBERSHIPS
    NODE_SOCKET_SET_MACRO(env, exports, IP_MAX_MEMBERSHIPS);
#endif
#ifdef  IP_TRANSPARENT
    NODE_SOCKET_SET_MACRO(env, exports, IP_TRANSPARENT);
#endif

    /* IPv6 [gs]etsockopt options, defined in RFC2553 */
#ifdef  IPV6_JOIN_GROUP
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_JOIN_GROUP);
#endif
#ifdef  IPV6_LEAVE_GROUP
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_LEAVE_GROUP);
#endif
#ifdef  IPV6_MULTICAST_HOPS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_MULTICAST_HOPS);
#endif
#ifdef  IPV6_MULTICAST_IF
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_MULTICAST_IF);
#endif
#ifdef  IPV6_MULTICAST_LOOP
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_MULTICAST_LOOP);
#endif
#ifdef  IPV6_UNICAST_HOPS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_UNICAST_HOPS);
#endif
    /* Additional IPV6 socket options, defined in RFC 3493 */
#ifdef IPV6_V6ONLY
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_V6ONLY);
#endif
    /* Advanced IPV6 socket options, from RFC 3542 */
#ifdef IPV6_CHECKSUM
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_CHECKSUM);
#endif
#ifdef IPV6_DONTFRAG
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_DONTFRAG);
#endif
#ifdef IPV6_DSTOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_DSTOPTS);
#endif
#ifdef IPV6_HOPLIMIT
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_HOPLIMIT);
#endif
#ifdef IPV6_HOPOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_HOPOPTS);
#endif
#ifdef IPV6_NEXTHOP
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_NEXTHOP);
#endif
#ifdef IPV6_PATHMTU
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_PATHMTU);
#endif
#ifdef IPV6_PKTINFO
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_PKTINFO);
#endif
#ifdef IPV6_RECVDSTOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RECVDSTOPTS);
#endif
#ifdef IPV6_RECVHOPLIMIT
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RECVHOPLIMIT);
#endif
#ifdef IPV6_RECVHOPOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RECVHOPOPTS);
#endif
#ifdef IPV6_RECVPKTINFO
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RECVPKTINFO);
#endif
#ifdef IPV6_RECVRTHDR
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RECVRTHDR);
#endif
#ifdef IPV6_RECVTCLASS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RECVTCLASS);
#endif
#ifdef IPV6_RTHDR
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RTHDR);
#endif
#ifdef IPV6_RTHDRDSTOPTS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RTHDRDSTOPTS);
#endif
#ifdef IPV6_RTHDR_TYPE_0
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RTHDR_TYPE_0);
#endif
#ifdef IPV6_RECVPATHMTU
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_RECVPATHMTU);
#endif
#ifdef IPV6_TCLASS
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_TCLASS);
#endif
#ifdef IPV6_USE_MIN_MTU
    NODE_SOCKET_SET_MACRO(env, exports, IPV6_USE_MIN_MTU);
#endif

    /* TCP options */
#ifdef  TCP_NODELAY
    NODE_SOCKET_SET_MACRO(env, exports, TCP_NODELAY);
#endif
#ifdef  TCP_MAXSEG
    NODE_SOCKET_SET_MACRO(env, exports, TCP_MAXSEG);
#endif
#ifdef  TCP_CORK
    NODE_SOCKET_SET_MACRO(env, exports, TCP_CORK);
#endif
#ifdef  TCP_KEEPIDLE
    NODE_SOCKET_SET_MACRO(env, exports, TCP_KEEPIDLE);
#endif
#ifdef  TCP_KEEPINTVL
    NODE_SOCKET_SET_MACRO(env, exports, TCP_KEEPINTVL);
#endif
#ifdef  TCP_KEEPCNT
    NODE_SOCKET_SET_MACRO(env, exports, TCP_KEEPCNT);
#endif
#ifdef  TCP_SYNCNT
    NODE_SOCKET_SET_MACRO(env, exports, TCP_SYNCNT);
#endif
#ifdef  TCP_LINGER2
    NODE_SOCKET_SET_MACRO(env, exports, TCP_LINGER2);
#endif
#ifdef  TCP_DEFER_ACCEPT
    NODE_SOCKET_SET_MACRO(env, exports, TCP_DEFER_ACCEPT);
#endif
#ifdef  TCP_WINDOW_CLAMP
    NODE_SOCKET_SET_MACRO(env, exports, TCP_WINDOW_CLAMP);
#endif
#ifdef  TCP_INFO
    NODE_SOCKET_SET_MACRO(env, exports, TCP_INFO);
#endif
#ifdef  TCP_QUICKACK
    NODE_SOCKET_SET_MACRO(env, exports, TCP_QUICKACK);
#endif
#ifdef  TCP_FASTOPEN
    NODE_SOCKET_SET_MACRO(env, exports, TCP_FASTOPEN);
#endif
#ifdef  TCP_CONGESTION
    NODE_SOCKET_SET_MACRO(env, exports, TCP_CONGESTION);
#endif
#ifdef  TCP_USER_TIMEOUT
    NODE_SOCKET_SET_MACRO(env, exports, TCP_USER_TIMEOUT);
#endif
#ifdef  TCP_NOTSENT_LOWAT
    NODE_SOCKET_SET_MACRO(env, exports, TCP_NOTSENT_LOWAT);
#endif

    /* IPX options */
#ifdef  IPX_TYPE
    NODE_SOCKET_SET_MACRO(env, exports, IPX_TYPE);
#endif

/* Reliable Datagram Sockets */
#ifdef RDS_CMSG_RDMA_ARGS
    NODE_SOCKET_SET_MACRO(env, exports, RDS_CMSG_RDMA_ARGS);
#endif
#ifdef RDS_CMSG_RDMA_DEST
    NODE_SOCKET_SET_MACRO(env, exports, RDS_CMSG_RDMA_DEST);
#endif
#ifdef RDS_CMSG_RDMA_MAP
    NODE_SOCKET_SET_MACRO(env, exports, RDS_CMSG_RDMA_MAP);
#endif
#ifdef RDS_CMSG_RDMA_STATUS
    NODE_SOCKET_SET_MACRO(env, exports, RDS_CMSG_RDMA_STATUS);
#endif
#ifdef RDS_CMSG_RDMA_UPDATE
    NODE_SOCKET_SET_MACRO(env, exports, RDS_CMSG_RDMA_UPDATE);
#endif
#ifdef RDS_RDMA_READWRITE
    NODE_SOCKET_SET_MACRO(env, exports, RDS_RDMA_READWRITE);
#endif
#ifdef RDS_RDMA_FENCE
    NODE_SOCKET_SET_MACRO(env, exports, RDS_RDMA_FENCE);
#endif
#ifdef RDS_RDMA_INVALIDATE
    NODE_SOCKET_SET_MACRO(env, exports, RDS_RDMA_INVALIDATE);
#endif
#ifdef RDS_RDMA_USE_ONCE
    NODE_SOCKET_SET_MACRO(env, exports, RDS_RDMA_USE_ONCE);
#endif
#ifdef RDS_RDMA_DONTWAIT
    NODE_SOCKET_SET_MACRO(env, exports, RDS_RDMA_DONTWAIT);
#endif
#ifdef RDS_RDMA_NOTIFY_ME
    NODE_SOCKET_SET_MACRO(env, exports, RDS_RDMA_NOTIFY_ME);
#endif
#ifdef RDS_RDMA_SILENT
    NODE_SOCKET_SET_MACRO(env, exports, RDS_RDMA_SILENT);
#endif

    /* get{addr,name}info parameters */
#ifdef EAI_ADDRFAMILY
    NODE_SOCKET_SET_MACRO(env, exports, EAI_ADDRFAMILY);
#endif
#ifdef EAI_AGAIN
    NODE_SOCKET_SET_MACRO(env, exports, EAI_AGAIN);
#endif
#ifdef EAI_BADFLAGS
    NODE_SOCKET_SET_MACRO(env, exports, EAI_BADFLAGS);
#endif
#ifdef EAI_FAIL
    NODE_SOCKET_SET_MACRO(env, exports, EAI_FAIL);
#endif
#ifdef EAI_FAMILY
    NODE_SOCKET_SET_MACRO(env, exports, EAI_FAMILY);
#endif
#ifdef EAI_MEMORY
    NODE_SOCKET_SET_MACRO(env, exports, EAI_MEMORY);
#endif
#ifdef EAI_NODATA
    NODE_SOCKET_SET_MACRO(env, exports, EAI_NODATA);
#endif
#ifdef EAI_NONAME
    NODE_SOCKET_SET_MACRO(env, exports, EAI_NONAME);
#endif
#ifdef EAI_OVERFLOW
    NODE_SOCKET_SET_MACRO(env, exports, EAI_OVERFLOW);
#endif
#ifdef EAI_SERVICE
    NODE_SOCKET_SET_MACRO(env, exports, EAI_SERVICE);
#endif
#ifdef EAI_SOCKTYPE
    NODE_SOCKET_SET_MACRO(env, exports, EAI_SOCKTYPE);
#endif
#ifdef EAI_SYSTEM
    NODE_SOCKET_SET_MACRO(env, exports, EAI_SYSTEM);
#endif
#ifdef EAI_BADHINTS
    NODE_SOCKET_SET_MACRO(env, exports, EAI_BADHINTS);
#endif
#ifdef EAI_PROTOCOL
    NODE_SOCKET_SET_MACRO(env, exports, EAI_PROTOCOL);
#endif
#ifdef EAI_MAX
    NODE_SOCKET_SET_MACRO(env, exports, EAI_MAX);
#endif
#ifdef AI_PASSIVE
    NODE_SOCKET_SET_MACRO(env, exports, AI_PASSIVE);
#endif
#ifdef AI_CANONNAME
    NODE_SOCKET_SET_MACRO(env, exports, AI_CANONNAME);
#endif
#ifdef AI_NUMERICHOST
    NODE_SOCKET_SET_MACRO(env, exports, AI_NUMERICHOST);
#endif
#ifdef AI_NUMERICSERV
    NODE_SOCKET_SET_MACRO(env, exports, AI_NUMERICSERV);
#endif
#ifdef AI_MASK
    NODE_SOCKET_SET_MACRO(env, exports, AI_MASK);
#endif
#ifdef AI_ALL
    NODE_SOCKET_SET_MACRO(env, exports, AI_ALL);
#endif
#ifdef AI_V4MAPPED_CFG
    NODE_SOCKET_SET_MACRO(env, exports, AI_V4MAPPED_CFG);
#endif
#ifdef AI_ADDRCONFIG
    NODE_SOCKET_SET_MACRO(env, exports, AI_ADDRCONFIG);
#endif
#ifdef AI_V4MAPPED
    NODE_SOCKET_SET_MACRO(env, exports, AI_V4MAPPED);
#endif
#ifdef AI_DEFAULT
    NODE_SOCKET_SET_MACRO(env, exports, AI_DEFAULT);
#endif
#ifdef NI_MAXHOST
    NODE_SOCKET_SET_MACRO(env, exports, NI_MAXHOST);
#endif
#ifdef NI_MAXSERV
    NODE_SOCKET_SET_MACRO(env, exports, NI_MAXSERV);
#endif
#ifdef NI_NOFQDN
    NODE_SOCKET_SET_MACRO(env, exports, NI_NOFQDN);
#endif
#ifdef NI_NUMERICHOST
    NODE_SOCKET_SET_MACRO(env, exports, NI_NUMERICHOST);
#endif
#ifdef NI_NAMEREQD
    NODE_SOCKET_SET_MACRO(env, exports, NI_NAMEREQD);
#endif
#ifdef NI_NUMERICSERV
    NODE_SOCKET_SET_MACRO(env, exports, NI_NUMERICSERV);
#endif
#ifdef NI_DGRAM
    NODE_SOCKET_SET_MACRO(env, exports, NI_DGRAM);
#endif

    /* shutdown() parameters */
#ifdef SHUT_RD
    NODE_SOCKET_SET_MACRO(env, exports, SHUT_RD);
#elif defined(SD_RECEIVE)
    NODE_SOCKET_SET_CONSTANT(exports, "SHUT_RD", SD_RECEIVE);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "SHUT_RD", 0);
#endif
#ifdef SHUT_WR
    NODE_SOCKET_SET_MACRO(env, exports, SHUT_WR);
#elif defined(SD_SEND)
    NODE_SOCKET_SET_CONSTANT(exports, "SHUT_WR", SD_SEND);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "SHUT_WR", 1);
#endif
#ifdef SHUT_RDWR
    NODE_SOCKET_SET_MACRO(env, exports, SHUT_RDWR);
#elif defined(SD_BOTH)
    NODE_SOCKET_SET_CONSTANT(exports, "SHUT_RDWR", SD_BOTH);
#else
    NODE_SOCKET_SET_CONSTANT(exports, "SHUT_RDWR", 2);
#endif

// }

// NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
    return exports;
}

NODE_API_MODULE(fibonacci, Init);

}