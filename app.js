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


// dummy
setInterval(function(){},1000*1000)