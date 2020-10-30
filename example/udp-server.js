const socket = require('./lib/socket');

const fd = socket.socket(socket.ProtocolFamily.AF_INET, socket.types.SOCK_DGRAM,0);

if(fd < -1){
  return;
}

const addr = {sin_family: 2, sin_port: 8888, s_addr: 0xc0a81f58};

const bindResult = socket.bind(fd, addr);


if(bindResult < -1){
  return;
}

while(1){
  const bf = new Uint8Array(100);
  const len = socket.recvfrom(fd, bf, 100, 0, addr);
  console.log(len);
  console.log(String.fromCharCode(...bf));
}


