const socket = require('../../lib/socket');

const server = socket.socket(socket.AF_INET,socket.SOCK_STREAM, 0);

process.on('beforeExit', () => {
  console.log('on exit');
  server.close();
});

server.bind(8881);

const result = server.listen();
console.log('1,',result);

while(result){
  const client = server.accept();
  console.log("2,",client);
  while(client){
    const buffer = client.recv(200);
    if(buffer){
      console.log(String.fromCharCode(...buffer))
    }else{
      break;
    }
  }

}

