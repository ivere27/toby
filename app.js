'ust strict'

const toby = process.binding('toby');
console.log(toby.hello());

console.log(toby.callback());

setTimeout(function(){
  var ret = toby.callback(function(x){
    console.log(`${x} in callback`);
    return x;
  });
  console.log(ret); // x
},500);


// assgined from example.cpp
console.log(`__val = ${__val}`);

var num = 42;
var foo = 'foo';
global.bar = function(x) {
  return `${foo} bar ${x}`;
}

var result = toby.call('key', {num, foo});
console.log(`result from toby.call = ${result}`);


//setInterval(function(){},1000); // dummy event

return; // exit the scope. atExitCB