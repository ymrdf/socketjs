const socket = require('../../lib/socket');

const cSocket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_UDP);

const a = 1;
const res = cSocket.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, a);

const bf = new Uint8Array([69,0,61,0,0,0,0,0,64,17,0,0,192,168,31,51,192,168,31,51,34,184,34,185,0,41,136,168,216,203,1,0,0,1,0,0,0,0,0,0,3,119,119,119,8,99,104,111,110,103,102,101,114,2,99,110,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,73,73,73,73,73,73,73,73,73,73,73,73,73,73]);

const len= cSocket.sendto({address:'0.0.0.0', port: 8888}, bf);

