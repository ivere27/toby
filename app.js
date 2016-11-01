'ust strict'

const soy = process.binding('soy');
console.log(soy.hello());

console.log(soy.add());
console.log(soy.add());

console.log(soy.callback());

setTimeout(function(){
  var ret = soy.callback(function(x){
    console.log(`${x} in callback`);
    return x;
  });
  console.log(ret); // x
},500);


soy.compile()
console.log(`__val = ${__val}`);

var num = 42;
var foo = 'foo';
global.bar = function(x) {
  return `${foo} bar ${x}`;
}

console.log(soy.globalGet());

setInterval(function(){},1000); // dummy event

//return; // exit the scope. atExitCB