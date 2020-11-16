const socket = require('../../lib/socket');

const server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM,0);
const bindResult = server.bind(8888);

while(bindResult){
  const buffer = server.recv(200);
  console.log(buffer)
}




// const bf = new Uint8Array(100);
// const a = socket.asyncRecvfrom(fd, bf.buffer, 100, 0, (err, res) => {
//   console.log(err,res);
//   console.log(String.fromCharCode(...new Uint8Array(bf)));
// })

// console.log(a);
