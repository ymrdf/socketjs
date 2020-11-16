const bindings = require('bindings')('socket.node');
const utils = require('./utils');


bindings.asyncAccept = (fd, cb) => {
  bindings._asyncAccept(fd,(err, result) => {
    bindings.asyncAccept(fd,cb);
    const { fd:clientFd, addr } = result;
    const client = new Socket(clientFd, addr.sin_family);
    client.address = addr.s_addr;
    client.port = addr.sin_port;
    cb(err,client);
  });
};

bindings.asyncRecv = (fd, buffer, buflen,flag, cb) => {
  bindings._asyncRecv(fd, buffer.buffer, buflen,flag,(err,len) => {
    bindings.asyncRecv(fd, buffer, buflen,flag,cb);
    cb(err, buffer.slice(0, len));
  });
};

bindings.asyncRecvfrom = (fd, buffer, buflen,flag, cb) => {
  bindings._asyncRecvfrom(fd, buffer.buffer, buflen,flag,(err,result) => {
    bindings.asyncRecvfrom(fd, buffer, buflen,flag,cb);
    const {length, fromAddr} = result;
    cb(err,{length, fromAddr, buffer: buffer.slice(0, length)});
  });
};

bindings.socket = ( protocolFamily = bindings.AF_INET, socketType = bindings.SOCK_STREAM, protocol =  0) => {
  const id = bindings._socket(protocolFamily, socketType, protocol);
  if(id >=0){
    return new Socket(id,protocolFamily, socketType, protocol);
  }else{
    throw new Error('Error: creat socket');
  }
}

const _ID = Symbol('id');

const _DEFAULT_BUFFER_LEN = 2048;

const _DEFAULT_RECV_FLAG = 0;

const _DEFAULT_SEND_FLAG = 0;

class Socket{

  constructor(id, protocolFamily = bindings.AF_INET, socketType = bindings.SOCK_STREAM, protocol =  0){
    this[_ID] = null;
    this.protocolFamily = null;
    this.socketType = null;
    this.protocol = null;
    this.address = null;
    this.port = null;
    this.bindStatus = false;
    this.listenStatus = false;
    this.closeStatus = false;
    this.connectStatus = false;
    this.error = false;
    this.errorType = '';
    this.errorMsg = null;
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

  verify(arg,argType,errtype, msg) {
    if(typeof arg !== argType){
      this.dealError(errtype, msg);
    }
  }
  
  close() {
    const result = bindings._close(this[_ID]);
    if(result >= 0){
      this.closeStatus = true;
      this.init();
    }else{
      this.closeStatus = false;
      this.dealError('close');
    }
  }

  shutdown(how) {
    this.verify(level, 'number', 'shutdown', 'argument 1 should be number');

    const result = bindings._shutdown(this[_ID], how);
    if(result < 0){
      this.dealError('shutdown');
    }

    return result;
  }

  bind(port, address = bindings.INADDR_ANY){
    if(typeof address !== 'number'){
      if(typeof address === 'string'){
        address = utils.createIntIp(address);
      }else{
        this.dealError('bind', 'Ip address is wrong:' + address);
      }
    }
    const addr = {sin_family: this.protocolFamily, sin_port: port, s_addr: address}
    const bindStatus = bindings._bind(this[_ID], addr);
    if(bindStatus >= 0){
      this.bindStatus = true;
      this.address = address;
      this.port = port;
    }else{
      this.bindStatus = false;
      this.dealError('Bind')
    }

    return this.bindStatus;
  }

  listen(flag = 128){
    if(typeof flag !== 'number' || flag < 0){
      flag = 0;
    }
    const result = bindings._listen(this[_ID], flag);
    if(result >= 0){
      this.listenStatus = true;
    }else{
      this.listenStatus = false;
      this.dealError('listen');
    }

    return this.listenStatus;
  }

  accept() {
    const { fd, addr } = bindings._accept(this[_ID]);
    if(fd >= 0){
      const client = new Socket(fd, addr.sin_family,this.socketType,this.protocol);
      client.address = addr.s_addr;
      client.port = addr.sin_port;
      return client;
    }else{
      this.dealError('Accept');
    }
  }

  asyncAccept(cb){
    if(typeof cb !== 'function'){
      this.dealError('asyncAccept', 'argument 1 is wrong:' + cb);
    }
    return bindings.asyncAccept(this[_ID], cb);
  }

  recv(bufsize = _DEFAULT_BUFFER_LEN, flag = _DEFAULT_RECV_FLAG) {
    const bf = new Uint8Array(bufsize);
    const len = bindings._recv(this[_ID], bf.buffer, bufsize, flag);
    if(bf <= 0){
      this.close();
      return null;
    }else{
      return bf.slice(0, len);
    }
  }

  recvfrom(bufsize = _DEFAULT_BUFFER_LEN, flag = _DEFAULT_RECV_FLAG) {
    const bf = new Uint8Array(bufsize);
    const result = bindings._recvfrom(this[_ID], bf.buffer, bufsize, flag);
    if(result.length < 0){
      this.close();
      return null;
    }else{
      return result;
    }
  }

  asyncRecv(bufsize = _DEFAULT_BUFFER_LEN, flag = _DEFAULT_RECV_FLAG, cb){
    if(typeof bufsize != 'number'){
      this.dealError('asyncRecv', 'argument 1 is wrong:' + bufsize);
    }
    const bf = new Uint8Array(bufsize);
    return bindings.asyncRecv(this[_ID], bf, bufsize, flag, cb);
  }

  asyncRecvfrom(bufsize = _DEFAULT_BUFFER_LEN, flag = _DEFAULT_RECV_FLAG, cb){
    if(typeof bufsize != 'number'){
      this.dealError('asyncRecv', 'argument 1 is wrong:' + bufsize);
    }
    const bf = new Uint8Array(bufsize);
    return bindings.asyncRecvfrom(this[_ID], bf.buffer, bufsize, flag, cb);
  }

  connect(addrOb) {
    let { address, port } = addrOb;

    if(typeof address !== 'number'){
      if(typeof address === 'string'){
        address = utils.createIntIp(address);
      }else{
        this.dealError('connect', 'argument Ip address is wrong:' + address);
      }
    }
    if(!port || typeof port !== 'number'){
      this.dealError('connect', 'argument port is wrong:' + port);
    }
    const addr = {sin_family: this.protocolFamily, sin_port: port, s_addr: address}
    const result = bindings._connect(this[_ID],addr);
    if(result < 0){
      this.connectStatus = false;
      this.dealError('connect'); 
    }else{
      this.connectStatus = true;
    }
    return result;
  }

  send(buf, len, flag = _DEFAULT_SEND_FLAG) {
    if(buf.buffer){
      buf = buf.buffer;
    }

    if(!len){
      len = buf.byteLength;
    }

    const result = bindings._send(this[_ID], buf, len, flag);
    if(result <= 0){
      this.dealError('send'); 
    }

    return result;
  }

  sendto(addrOb, buf, len, flag = _DEFAULT_SEND_FLAG) {
    let { address, port } = addrOb;

    if(typeof address !== 'number'){
      if(typeof address === 'string'){
        address = utils.createIntIp(address);
      }else{
        this.dealError('connect', 'argument Ip address is wrong:' + address);
      }
    }
    if(!port || typeof port !== 'number'){
      this.dealError('connect', 'argument port is wrong:' + port);
    }

    const addr = {sin_family: this.protocolFamily, sin_port: port, s_addr: address}

    if(buf.buffer){
      buf = buf.buffer;
    }

    if(!len){
      len = buf.byteLength;
    }

    const result = bindings._sendto(this[_ID], buf, len, flag, addr);
    if(result < 0){
      this.dealError('send'); 
    }
    
    return result;
  }

  setsockopt(level, optname, value){
    const result = bindings._setsockopt(this[_ID], level, optname, value);

    if(result < 0){
      this.dealError('setsockopt'); 
    }
    
    return result;
  }

  getsockopt(level, optname, len = 0){
    this.verify(level, 'number', 'getsockopt', 'argument 1 should be number');
    this.verify(optname, 'number', 'getsockopt', 'argument 2 should be number');
    this.verify(len, 'number', 'getsockopt', 'argument 3 should be number');

    const res = bindings._getsockopt(this[_ID], level, optname, len);

    if(res.result < 0){
      this.dealError('getsockopt');
    }
    
    return res;
  }

  getsockname(){

    const res = bindings._getsockname(this[_ID]);

    if(res.result < 0){
      this.dealError('getsockname');
    }
    
    return res;
  }

  getpeername(){
    const res = bindings._getsockname(this[_ID]);
    if(res.result < 0){
      this.dealError('getpeername');
    }
    
    return res;
  }
 
}

module.exports = bindings;