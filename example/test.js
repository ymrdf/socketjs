const addon = require('../lib/socket');
addon.asyncAccept = (fd, cb) => {
  addon._asyncAccept(fd,(err,client) => {
    addon.asyncAccept(fd,cb);
    cb(err,client);
  });
};

addon.asyncRecv = (fd, buffer, buflen,flag, cb) => {
  addon._asyncRecv(fd, buffer, buflen,flag,(err,client) => {
    addon.asyncRecv(fd, buffer, buflen,flag,cb);
    cb(err,client);
  });
};


const fd = addon.socket(2,1, 0);

process.on('beforeExit', () => {
  console.log('on exit');
  addon.close(fd);
});
process.on('exit', () => {
  console.log('on exit');
});
console.log(fd);

const a = addon.bind(fd, {sin_family: 2, sin_port: 8890, s_addr: 0x00000000});
console.log(`bind: ${a} `)
const b = addon.listen(fd, 1);
console.log(`listen: ${b} `)

// const server = () => {
//   const c = addon.accept(fd);
//   console.log(`accept: ${c} `)
//   const bf = new Uint8Array(100);
//   addon.asyncRecv(c, bf.buffer, 100, 0, (err,len ) => {
//     console.log('asyncRecv:', len, err);
//     if(len<=0 || err) {
//       addon.close(c);
//       return;
//     }
//     console.log(len);
//     console.log(String.fromCharCode(...bf.slice(0,len)))
//   })
//   let i = 1;
//   // while(i){
//   //   const d = addon.recv(c, bf.buffer, 100, 0);
//   //   console.log(d);
//   //   if(d<=0) {
//   //     addon.close(c);
//   //     server();
//   //     break;
//   //   }
//   //   console.log(d);
//   //   console.log(String.fromCharCode(...bf.slice(0,d)))
//   // }
// }



const server = () => {
  addon.asyncAccept(fd, (e,c) =>{
    
    if(e){
      console.log('async accept error:', e);
      return;
    }


    console.log(`accept: ${c} `)
    const bf = new Uint8Array(100);

    addon.asyncRecv(c, bf.buffer, 100, 0, (err,len ) => {
      console.log('asyncRecv:', len, err);
      if(len<=0 || err) {
        addon.close(c);
        return;
      }
      console.log(len);
      console.log(String.fromCharCode(...bf.slice(0,len)))
    })

  });

  console.log('end');

}

server();
