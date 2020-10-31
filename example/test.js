const addon = require('../lib/socket');
const fd = addon.socket(2,1, 0);

process.on('beforeExit', () => {
  console.log('on exit');
  addon.close(fd);
});
process.on('exit', () => {
  console.log('on exit');
  addon.close(fd);
});
console.log(fd);

const a = addon.bind(fd, {sin_family: 2, sin_port: 8890, sin_addr: 0x7f000001});
console.log(`bind: ${a} `)
const b = addon.listen(fd, 1);
console.log(`listen: ${b} `)

const server = () => {
  const c = addon.accept(fd);
  console.log(`accept: ${b} `)
  const bf = new Uint8Array(100);
  let i = 1;
  while(i){
    const d = addon.recv(c, bf, 100, 0);
    console.log(d);
    if(d<=0) {
      addon.close(c);
      server();
      break;
    }
    console.log(d);
    console.log(String.fromCharCode(...bf.slice(0,d)))
  }
}

server();
