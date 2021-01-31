
const socket = require('../../lib/socket');

console.log(socket.AF_PACKET);

console.log(socket.AF_INET,socket.AF_PACKET, socket.SOCK_RAW, socket.IPPROTO_IP);
const sSocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_IP);
sSocket.bind(0)

const aa = sSocket.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1);

sSocket.ioctl(socket.SIO_RCVALL, socket.RCVALL_ON);



console.log('set opt:', aa);

// while(1){
  const result = sSocket.recvfrom(200);

  var v2 = new Uint8Array(result, 0);

  console.log(v2);
// }
sSocket.ioctl(socket.SIO_RCVALL, socket.RCVALL_ON);

