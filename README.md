# sgmon
A small and easy-to-use module(cross out, it's mon) to grab system statistics through libstatgrab. Statistics include CPU, processes, load, memory, swap, network I/O, disk I/O (and file system information). It could be used for monitoring the system for alerting or graphing purposes.

Now support linux only.

## Installation
```
npm install sgmon
```
Uuuuum, it needs `node-gyp` to build. Failing to install automaticly, maybe you should:
```
npm install node-gyp -g
```
## Usage
```js
var sgmon = require('sgmon')

console.log(sgmon.grab())
// grab system statistics, and return a object

sgmon.nap(function (err, stats) {
  if(err)
    console.log(err)
  console.log(stats)
}, 3000) 
// sgmon.nap(callback[,delay])
// grab and invoke callback every delay(ms, defaults 3000)
```
The return likes:
```js
{ host_info: { hostname: 'booooom', uptime: 6858472 },
  load_stats: { min1: 0.07, min5: 0.03, min15: 0.05 },
  cpu: 
    { idle: 99.03449934457036,
      system: 0.942248478804679,
      user: 0.5937322369309994 },
  mem: { total: 1039847424, used: 613339136, free: 426508288 },
  swap: { total: 0, used: 0, free: 0 },
  disk: 
    [ { name: 'xvda', read: 182, write: 44 },
      { name: 'xvda1', read: 182, write: 44 }...so on.
```
## Else...
I can't make libstatgrab staticly, and just link the dynamic library at `./addon/lib/libstatgrab.so`.
So, it brings some problems...
Compatibility? Security?


If this dynamic library don't work, you can try to install `libstatgrab-dev` by package manager and copy `libstatgrab.so` to `./addon/lib/`.


The libstatgrab: [http://www.i-scream.org/libstatgrab/](http://www.i-scream.org/libstatgrab/)

## License
GPL-2.0
