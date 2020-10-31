const readline = require('readline');
  
const stringToUint8Array = (str) => {
  const result = new Uint8Array(str.length).fill(0);
  for(let i = 0; i < str.length; i++){
    result[i] = str.charCodeAt(i);
  }
  console.log(result);
  return result;
}

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});


const addon = require('../lib/socket');

const fd = addon.socket(2,1,0);
const a = addon.connect(fd, {sin_family: 2, sin_port: 8890, s_addr: 0x7f000001});

console.log("connect result: ", fd , "  ", a);
const send = (response) => {
  const bf = stringToUint8Array(response);
  addon.send(fd, bf.buffer, bf.length, 0);
}

const read = () => {
  rl.question('你如何看待 Node.js 中文网？', (answer, a ,b ) => {
    send(answer);
    read();
  });
}

read();
