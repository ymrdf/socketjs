const addon = require('../lib/socket');

const fd = addon.socket(2,1, 0);

process.on('beforeExit', () => {
  console.log('on exit');
  addon.close(fd);
});
process.on('exit', () => {
  console.log('on exit');
});

addon.bind(fd, {sin_family: 2, sin_port: 8890, s_addr: 0x00000000});
addon.listen(fd, 1);

const server = () => {
  addon.asyncAccept(fd, (e,c) =>{
    
    if(e){
      console.log('async accept error:', e);
      return;
    }

    const bf = new Uint8Array(100);

    addon.asyncRecv(c, bf.buffer, 100, 0, (err,len ) => {
      if(len<=0 || err) {
        addon.close(c);
        return;
      }
      console.log(String.fromCharCode(...bf.slice(0,len)))
    })

  });

  console.log('end');

}

server();
