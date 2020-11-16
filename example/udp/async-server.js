const socket = require('../../lib/socket');

const server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM,0);
const bindResult = server.bind(8888);

if(bindResult){
  server.asyncRecv(100, 0, (err, result ) => {
    if(err) {
      console.log('error');
      server.close(c);
      return;
    }
    console.log(String.fromCharCode(...result));
  })
}


