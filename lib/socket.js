var bindings = require('bindings')('socket.node')

console.log(bindings['hello'],bindings["SOCK_RAW"])
const a = ['SHUT_RDWR','AF_ROUTE','AF_LINK','AF_UNSPEC','PF_SYSTEM','AF_AX25'];
console.log(['AF_ROUTE','AF_LINK','AF_UNSPEC','PF_SYSTEM'])

a.forEach(item => {
  console.log(item,':',bindings[item])
})

module.exports = bindings;