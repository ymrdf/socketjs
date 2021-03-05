
const socket = require('../../lib/socket');

const sSocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_IP);
sSocket.bind(0)

const aa = sSocket.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1);

sSocket.ioctl(socket.SIO_RCVALL, socket.RCVALL_ON);

const result = sSocket.recvfrom(200);

var v2 = new Uint8Array(result, 0);

console.log(v2);

sSocket.ioctl(socket.SIO_RCVALL, socket.RCVALL_OFF);

