const BYTE_LENGTH = 8;

const getValue = (bf, start, length ) => {

}

export const setValue =  (bf, startBufferIndex, value ) => {

  value = value.replace(/\s+/g, '');
  const start = startBufferIndex - 1;
  const startByteIndex = Math.floor(start / BYTE_LENGTH);
  const firstByteValueLength = 8 - start % BYTE_LENGTH;
  const firstByteValue = value.slice(0, firstByteValueLength);
  const firstByte = Number('0b' + firstByteValue);

  const endBufferIndex = start + value.length;
  const endByteIndex = Math.floor(endBufferIndex / BYTE_LENGTH);
  const endByteValueLength = endBufferIndex % BYTE_LENGTH;
  const endByteString = value.slice(-endByteValueLength );
  const endByte = Number('0b' + endByteString) << ( 8 - endByteValueLength);

  const compositionInterFirstByte  = (bf[startByteIndex] >> firstByteValueLength << firstByteValueLength) + firstByte;
  //const compositionInterEndByte = (bf[endByteIndex]  << ( 8 - endByteString.length) >> ( 8 - endByteString.length)) + endByte;
  const compositionInterEndByte = Number('0b' + endByteString + bf[endByteIndex].toString(2).slice(endByteValueLength));
    
  for(let i = startByteIndex; i <= endByteIndex; i ++){
    if(i === startByteIndex){
      bf[i] = compositionInterFirstByte;
    }else if( i === endByteIndex){
      bf[i] = compositionInterEndByte;
    }else{
      const curByteStart = (i - startByteIndex - 1) * BYTE_LENGTH + firstByteValueLength;
      const curByte = value.slice(curByteStart, curByteStart + BYTE_LENGTH)
      bf[i] = Number('0b' + curByte);
    }
  }
}

// const bf1 = new Uint8Array(100);
// const bf2 = new Uint8Array(100).fill(0b11111111);
// // setValue(bf1, 3, '1111 1100 0001')
// // console.log(bf1);
// setValue(bf2, 9, '1111 1100 1111 1100 001')


// console.log(bf2);
