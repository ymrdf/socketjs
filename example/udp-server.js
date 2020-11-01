const socket = require('../lib/socket');

socket.asyncRecvfrom = (fd, buffer, buflen,flag, cb) => {
  socket._asyncRecvfrom(fd, buffer, buflen,flag,(err,client) => {
    socket.asyncRecvfrom(fd, buffer, buflen,flag,cb);
    cb(err,client);
  });
};

const fd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM,0);

if(fd < -1){
  return;
}

const addr = {sin_family: 2, sin_port: 8888, s_addr: 0xc0a81f58};

const bindResult = socket.bind(fd, addr);


if(bindResult < -1){
  return;
}

const bf = new Uint8Array(100);
const a = socket.asyncRecvfrom(fd, bf.buffer, 100, 0, (err, res) => {
  console.log(err,res);
  console.log(String.fromCharCode(...new Uint8Array(bf)));
})

console.log(a);
