const socket = require('../lib/socket');

message = socket.runFibonacciWorker(9990, (error, result) => {
  if (error) {
      console.log("worker return error")
  } else {
      console.log("result: ", result)
  }
})

console.log(message)
console.log(socket.AF_INET);
console.log(socket.SOL_UDP);
const fd = socket.socket(socket.AF_INET,socket.SOCK_DGRAM,0);
console.log('secket', fd);
const addr = {sin_family: 2, sin_port: 8888, s_addr: 0xc0a81ffb};

const bindResult = socket.bind(fd, addr);
console.log('bind:',bindResult);

console.log('close:', socket.close(fd));

