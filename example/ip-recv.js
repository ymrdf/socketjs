
const socket = require('../lib/socket');

const sSocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP);


const aa = sSocket.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1);

console.log('set opt:', aa);

while(1){
  const result = sSocket.recvfrom(200);
  console.log(result);
  // const bf2 = [];
  // console.log(bf);
  // bf.forEach(i => bf2.push(i.toString(2)));
  // console.log(bf2);
}

