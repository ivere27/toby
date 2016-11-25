'ust strict'

// assgined from example.cpp
console.log(`node :: _v = ${_v}`);

var num = 42;
var foo = 'foo';

toby.on('test', function(x){
  console.log(`node :: toby.on(test) = ${x}`);
});

var result = toby.hostCall('dory', {num, foo});
console.log(`node :: toby.hostCall() = ${result}`);

// return; // exit the scope. atExitCB