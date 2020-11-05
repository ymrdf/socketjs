const bindings = require('bindings')('socket.node');
const utils = require('./utils');


bindings.asyncAccept = (fd, cb) => {
  bindings._asyncAccept(fd,(err,client) => {
    bindings.asyncAccept(fd,cb);
    cb(err,client);
  });
};

bindings.asyncRecv = (fd, buffer, buflen,flag, cb) => {
  bindings._asyncRecv(fd, buffer, buflen,flag,(err,client) => {
    bindings.asyncRecv(fd, buffer, buflen,flag,cb);
    cb(err,client);
  });
};

bindings.asyncRecvfrom = (fd, buffer, buflen,flag, cb) => {
  bindings._asyncRecvfrom(fd, buffer, buflen,flag,(err,client) => {
    bindings.asyncRecvfrom(fd, buffer, buflen,flag,cb);
    cb(err,client);
  });
};

const socket = ( protocolFamily = bindings.AF_INET, socketType = bindings.SOCK_STREAM, protocol =  0) => {
  const id = bindings._socket(protocolFamily, socketType, protocol);
  if(id >=0){
    return new Socket(id,protocolFamily, socketType, protocol);
  }else{
    throw new Error('Error: creat socket');
  }
}

const _ID = Symbol('id');

const _DEFAULT_BUFFER_LEN = 2024;

const _DEFAULT_RECV_FLAG = 0;

class Socket{
  [_ID] = null;
  protocolFamily = null;
  socketType = null;
  protocol = null;

  address = null;
  port = null;


  bindStatus = false;
  listenStatus = false;
  closeStatus = false;

  error = false;
  errorType = '';
  errorMsg = null;

  constructor(id, protocolFamily = bindings.AF_INET, socketType = bindings.SOCK_STREAM, protocol =  0){
    this.protocolFamily = protocolFamily;
    this.socketType = socketType;
    this.protocol =  protocol;
    this[_ID] = id;
  }

  init() {
    this[_ID] = null;
    this.protocolFamily = null;
    this.socketType = null;
    this.protocol = null;
    this.address = null;
    this.port = null;
    this.bindStatus = false;
    this.listenStatus = false;
  }

  dealError(type, msg) {
    msg = msg || type + 'Error';
    this.errorType = type;
    this.errorStatus = true;
    this.errorMsg = msg;
    throw new Error(msg);
  }

  close() {
    const result = bindings._listen(this[_ID]);
    if(result >= 0){
      this.closeStatus = true;
      this.init();
    }else{
      this.listenStatus = false;
      this.dealError('close');
    }
  }

  bind(port, address = bindings.INADDR_ANY){
    if(typeof address !== 'number'){
      if(typeof address === 'string'){
        address = utils.createIntIp(address);
      }else{
        this.dealError('bind', 'Ip address is wrong:' + address);
      }
    }
    const addr = {sin_family: this[_PF], sin_port: port, s_addr: address}
    const bindStatus = bindings._bind(this[_ID], addr);
    if(bindStatus >= 0){
      this.bindStatus = true;
      this.address = address;
      this.port = port;
    }else{
      bindStatus = false;
      this.dealError('Bind')
    }
  }

  listen(flag = 128){
    if(typeof flag !== 'number' || flag < 0){
      flag = 0;
    }
    const result = bindings._listen(this[_ID], addr);
    if(result >= 0){
      this.listenStatus = true;
    }else{
      this.listenStatus = false;
      this.dealError('listen');
    }
  }

  accept() {
    const { fd, addr } = bindings._accept(this[_ID]);
    if(fd >= 0){
      const client = new Socket(fd, addr.sin_family,this.socketType,this.protocol);
      client.address = addr.s_addr;
      client.port = addr.sin_port;
    }else{
      this.dealError('Accept');
    }
  }

  recv(bufsize = _DEFAULT_BUFFER_LEN, flag = _DEFAULT_RECV_FLAG) {
    const bf = new Uint8Array(bufsize);
    const len = bindings._accept(this[_ID], bf, bufsize, flag);
    if(bf <= 0){
      this.close();
      return null;
    }else{
      return bf.slice(0, len);
    }
  }

}






module.exports = bindings;