# socket-api

Socket interface for node. It is available on linux, Windows, MacOS, and probably additional platforms.

##  contents

### Creating sockets

#### socket.socket(family=AF_INET, type=SOCK_STREAM, proto=0)
Create a new socket using the given address family, socket type and protocol number. The address family should be AF_INET ,The socket type should be SOCK_STREAM (the default), SOCK_DGRAM, SOCK_RAW or perhaps one of the other SOCK_ constants.The protocol number is usually zero and may be omitted.

```js
sock = socket.socket(
    socket.AF_INET,
    socket.SOCK_STREAM)
```

### Socket Objects
Socket objects have the following methods. 

#### socket.bind(port, address = 0x000000)
Bind the socket to address and port. The socket must not already be bound.

#### socket.accept()
Accept a connection. The socket must be bound to an address and listening for connections. The return value is a new socket object usable to send and receive data on the connection.

#### socket.asyncAccept(cb)
Async version of socket.accept, cb(err, client);

#### socket.listen(backlog = 128)
Enable a server to accept connections. If backlog is specified, it must be at least 0 (if it is lower, it is set to 0); it specifies the number of unaccepted connections that the system will allow before refusing new connections. If not specified, a default reasonable value is chosen.

#### socket.close()
Mark the socket closed.

#### socket.connect({address, port})
Connect to a remote socket at address.

#### socket.recv(bufsize = 2048, flags = 0)
Receive data from the socket. The return value is a Uint8Array representing the data received. The maximum amount of data to be received at once is specified by bufsize. See the Unix manual page recv(2) for the meaning of the optional argument flags; it defaults to zero.

#### socket.asyncRecv(bufsize = 2048, flags = 0, cb)
Async version of socket.recv, cb(err, buffer);

#### socket.recvfrom(bufsize = 2048, flags = 0)
Receive data from the socket. The return value is a object {length,buffer, fromAddr) where buffer is a Uint8Array representing the data received and fromAddr is the address of the socket sending the data. bufisize defaults to 2048, flags defaults to zero.

#### socket.asyncRecvfrom(bufsize = 2048, flags = 0, cb)
Async version of socket.recvfrom, cb(err, { length, buffer, fromAddr});

#### socket.send(buffer,length[, flags])
Send data to the socket. The socket must be connected to a remote socket. The optional flags argument has the same meaning as for recv() above. Returns the number of bytes sent. Applications are responsible for checking that all data has been sent; if only some of the data was transmitted, the application needs to attempt delivery of the remaining data. 

#### socket.sendto(buffer, flags, address)
Send data to the socket. The socket should not be connected to a remote socket, since the destination socket is specified by address. The optional flags argument has the same meaning as for recv() above. Return the number of bytes sent.

#### socket.setsockopt(level, optname, value: int)
#### socket.setsockopt(level, optname, value: buffer)
Set the value of the given socket option (see the Unix manual page setsockopt(2)). The needed symbolic constants are defined in the socket module (SO_* etc.). The value can be an integer or a buffer. In the later case it is up to the caller to ensure that the bytestring contains the proper bits.

#### socket.getsockopt(level, optname, buflen = 0)
Return the value of the given socket option . The needed symbolic constants (SO_* etc.) are defined in this module. A buffer is returned as a Uint8Array. It is up to the caller to decode the contents of the buffer .

#### socket.getpeername()
Return the remote address to which the socket is connected. This is useful to find out the port number of a remote IPv4/v6 socket, for instance. On some systems this function is not supported.

#### socket.getsockname()
Return the socket’s own address.

#### socket.shutdown(how)
Shut down one or both halves of the connection. If how is SHUT_RD, further receives are disallowed. If how is SHUT_WR, further sends are disallowed. If how is SHUT_RDWR, further sends and receives are disallowed.

## Example
Here are some minimal example programs using the TCP/IP protocol: a server that print all data that it receives, and a client using it. Note that a server must perform the sequence socket(), bind(), listen(), accept() (possibly repeating the accept() to service more than one client), while a client only needs the sequence socket(), connect(). Also note that the server does not sendall()/recv() on the socket it is listening on but on the new socket returned by accept().

tcp/server
```js
const socket = require('../../lib/socket');

const server = socket.socket(socket.AF_INET,socket.SOCK_STREAM, 0);

process.on('beforeExit', () => {
  console.log('on exit');
  server.close();
});

server.bind(8886);

const result = server.listen();

server.asyncAccept((error,client) =>{
  
  if(error){
    console.log('async accept error:', e);
    return;
  }

  const bf = new Uint8Array(100);

  client.asyncRecv(100, 0, (err, result ) => {
    if(err) {
      console.log('error');
      client.close(c);
      return;
    }
    console.log(String.fromCharCode(...result));
  })

});

```

tcp/client
```js
const readline = require('readline');
  
const stringToUint8Array = (str) => {
  const result = new Uint8Array(str.length).fill(0);
  for(let i = 0; i < str.length; i++){
    result[i] = str.charCodeAt(i);
  }
  console.log(result);
  return result;
}

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});


const socket = require('../../lib/socket');

const client = socket.socket(socket.AF_INET,socket.SOCK_STREAM, 0);
const connectResult = client.connect({port: 8886, address:'172.18.229.69' });

const send = (response) => {
  const bf = stringToUint8Array(response);
  client.send(bf, bf.length);
}

const read = () => {
  rl.question('client>:', (answer ) => {
    send(answer);
    read();
  });
}

read();

```


## tips

n-socket support IPv4 only;













