var bindings = require('bindings')('socket.node')

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

bindings.asyncRecvfrom = (fd, buffer, buflen,flag, cb) => {
  bindings._asyncRecvfrom(fd, buffer, buflen,flag,(err,client) => {
    bindings.asyncRecvfrom(fd, buffer, buflen,flag,cb);
    cb(err,client);
  });
};

module.exports = bindings;