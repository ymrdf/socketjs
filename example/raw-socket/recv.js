
const socket = require('../../lib/socket');

console.log(socket.AF_PACKET);

console.log(socket.AF_PACKET, socket.SOCK_RAW, socket.IPPROTO_IP);
const sSocket = socket.socket(2, socket.SOCK_RAW, socket.IPPROTO_ICMP);

console.log(socket.IPPROTO_IP, socket.IP_HDRINCL);
const aa = sSocket.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1);

console.log('set opt:', aa);

while(1){
  const result = sSocket.recvfrom(200);

  var v2 = new Uint8Array(result.buffer, 0);

  console.log(v2);
}

