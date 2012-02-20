assert  = require 'assert'
leveldb = require '../lib'

console.log 'Creating test database'
path = '/tmp/large.db'
db = leveldb.openSync path, create_if_missing: true

console.log 'Serializing and inserting 1,000,000 rows...'

start = Date.now();

for i in [0..1000000]
  key = "row#{i}"
  value = JSON.stringify
    index: i
    name: "Tim"
    age: 28
  db.putSync new Buffer(key), new Buffer(value)

delta = Date.now() - start;
console.log 'Completed in %d ms', delta
console.log '%s inserts per second', Math.floor(1000000000 / delta)

data = new Buffer 4096
data[i] = Math.floor Math.random() * 256 for i in [0..4096]

console.log 'Inserting the same 4k random bytes into 100,000 rows'
start = Date.now()
for i in [0..100000]
  db.putSync new Buffer("binary#{i}"), data

delta = Date.now() - start;
console.log 'Completed in %d ms', delta
console.log '%s inserts per second', Math.floor(1000000000 / delta)
