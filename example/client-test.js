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


const socket = require('../lib/socket');

const client = socket.socket(socket.AF_INET,socket.SOCK_STREAM, 0);
const connectResult = client.connect({port: 8881, address:'192.168.31.51' });

const send = (response) => {
  const bf = stringToUint8Array(response);
  client.send(bf, bf.length);
}

const read = () => {
  rl.question('client>:', (answer ) => {
    send(answer);
    read();
  });
}

read();
