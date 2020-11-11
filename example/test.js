const socket = require('../lib/socket');

const server = socket.socket(socket.AF_INET,socket.SOCK_STREAM, 0);

process.on('beforeExit', () => {
  console.log('on exit');
  server.close();
});

server.bind(8881);

const result = server.listen();

while(result){
  const client = server.accept();
  while(client){
    const buffer = client.recv(200);
    if(buffer){
      console.log(String.fromCharCode(...buffer))
    }else{
      break;
    }
  }

}

// const server = () => {
//   addon.asyncAccept(fd, (e,c) =>{
    
//     if(e){
//       console.log('async accept error:', e);
//       return;
//     }

//     const bf = new Uint8Array(100);

//     addon.asyncRecv(c, bf.buffer, 100, 0, (err,len ) => {
//       if(len<=0 || err) {
//         addon.close(c);
//         return;
//       }
//       console.log(String.fromCharCode(...bf.slice(0,len)))
//     })

//   });

//   console.log('end');
// }
