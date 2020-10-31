const socket = require('../lib/socket');

const fd = socket.socket(socket.ProtocolFamily.AF_INET, socket.types.SOCK_RAW, 17);

if(fd < -1){
  return;
}

const addr = {sin_family: 2, sin_port: 8889, sin_addr: 0xc0a81f58};


const aa = socket.setsockopt(fd, socket.sockoptLevels.IPPROTO_IP, socket.sockopts.IP_HDRINCL, 1);
// console.log(aa);
// const bf = new Uint8Array([
//   69,   0, 64,  0, 113, 74,   0,  0, 64,  1,  11, 93,
//  127,   0,  0,  1, 127,  0,   0,  1,  0,  0, 126, 56,
//   92, 225,  0,  1,  95, 47, 238, 81,  0,  1, 236, 95,
//    8,   9, 10, 11,  12, 13,  14, 15, 16, 17,  18, 19,
//   20,  21, 22, 23,  24, 25,  26, 27, 28, 29,  30, 31,
//   32,  33, 34, 35,  36, 37,  38, 39, 40, 41,  42, 43,
//   44,  45, 46, 47,  48, 49,  50, 51, 52, 53,  54, 55,
//    0,   0,  0,  0,   0,  0,   0,  0,  0,  0,   0,  0,
//    0,   0,  0,  0
// ]);
const bf = new Uint8Array(  [
  69,   0, 64,  0, 100, 183,  0,  0, 64,  1,  85, 241,
 192, 168, 31, 88, 192, 168, 31, 88,  0,  0,  73,  25,
 178, 232,  0,  3,  95,  48,  8, 73,  0, 13, 177, 113,
   8,   9, 10, 11,  12,  13, 14, 15, 16, 17,  18,  19,
  20,  21, 22, 23,  24,  25, 26, 27, 28, 29,  30,  31,
  32,  33, 34, 35,  36,  37, 38, 39, 40, 41,  42,  43,
  44,  45, 46, 47,  48,  49, 50, 51, 52, 53,  54,  55,
   0,   0,  0,  0,   0,   0,  0,  0,  0,  0,   0,   0,
   0,   0,  0,  0
]);
console.log(bf);
// const bf = new Uint8Array(100).fill(73);
const len = socket.sendto(fd, bf, bf.length, 0, addr);
console.log(len);

