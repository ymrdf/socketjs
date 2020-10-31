

const socket = require('../lib/socket');

const fd = socket.socket(socket.ProtocolFamily.AF_INET, socket.types.SOCK_RAW,6);

if(fd < -1){
  return;
}

const addr = {sin_family: 2, sin_port: 8889, s_addr: 0x00000000};

// const bindResult = socket.bind(fd, addr);

const aa = socket.setsockopt(fd, socket.sockoptLevels.IPPROTO_IP, socket.sockopts.IP_HDRINCL, 1);

console.log('set opt:', aa);
// if(bindResult < -1){
//   console.log('errrrrr')
//   return;
// }

while(1){
  const bf = new Uint8Array(100);
  const len = socket.recvfrom(fd, bf, 100, 0, addr);
  const bf2 = [];
  console.log(bf);
  bf.forEach(i => bf2.push(i.toString(2)));
  console.log(bf2);
  // const bf2 = bf.map(i => i.toString(2))
  // console.log(bf2);
  // console.log(len);
  // console.log(String.fromCharCode(...bf));
}

