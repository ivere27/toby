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


var foo = 'foo';
var bar = function(x) {
  return `${foo} bar ${x}`;
}


//return; // exit the scope. atExitCB