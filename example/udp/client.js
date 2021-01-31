const socket = require('../../lib/socket');

const client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM,0);
console.log(socket.SIO_RCVALL, socket.RCVALL_ON);

const bf = new Uint8Array(100).fill(73);

client.sendto({address:'192.168.31.251', port: 8888},bf);
