'use strict'

// print toby.version
console.log(`node :: toby.version = ${toby.version}`);

// assgined from example.cpp
console.log(`node :: _v = ${_v}`);

var num = 42;
var foo = 'foo';

toby.on('test', function(x){
  console.log(`node :: toby.on(test) = ${x}`);
});

var result = toby.hostCall('dory', {num, foo});
console.log(`node :: toby.hostCall() = ${result}`);

process.on('test', toby.hostOn('exit'))
process.on('exit', toby.hostOn('exit'));
//process.on('exit', function(code){console.log(`exit with ${code}`)});

process.emit('test', 'a', 20, {num, foo});

// exit after 2 secs
(function(){setTimeout(function(){
	process.exitCode = 42;
},2000)})();