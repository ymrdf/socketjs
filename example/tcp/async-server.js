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

console.log('end');