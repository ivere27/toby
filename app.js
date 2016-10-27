const soy = process.binding('soy');
console.log(soy.hello());

console.log(soy.add());
console.log(soy.add());

// dummy
setInterval(function(){},1000)