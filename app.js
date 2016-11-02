'ust strict'

const toby = process.binding('toby');
console.log(toby.hello());

console.log(toby.add());
console.log(toby.add());

console.log(toby.callback());

setTimeout(function(){
  var ret = toby.callback(function(x){
    console.log(`${x} in callback`);
    return x;
  });
  console.log(ret); // x
},500);


toby.compile()
console.log(`__val = ${__val}`);

var num = 42;
var foo = 'foo';
global.bar = function(x) {
  return `${foo} bar ${x}`;
}

console.log(toby.globalGet());


var _string = toby.toJson({num, foo});
console.log(`_string = ${_string}`);
console.log(`type of _string = ${typeof _string}`);


var _value = toby.setValue('key', {num, foo});
console.log(`value from example.cpp = ${_value}`);


//setInterval(function(){},1000); // dummy event

return; // exit the scope. atExitCB