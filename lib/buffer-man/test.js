var buffer = new ArrayBuffer(16);
var view = new DataView(buffer, 0);

view.setInt8(1, 42);
view.getInt8(1); // 42
console.log(buffer);