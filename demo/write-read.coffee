assert  = require 'assert'
leveldb = require '../lib'

console.log 'Creating test database'
path = '/tmp/wr.db'

batchSize = 100
totalSize = 100000

leveldb.open path, create_if_missing: true, (err, db) ->
  assert.ifError err

  start = Date.now()
  i = 0

  writeBenchStart = ->
    console.log 'Serializing and inserting 100,000 rows...'
    writeBench()

  writeBench = ->
    console.log "i = #{i}" if i % 10000 == 0
    batch = new(leveldb.Batch)
    for j in [0...batchSize]
      key = "row#{i}"
      value = JSON.stringify
        index: i
        name: "Tim"
        age: 28
      batch.put key, value
      ++i

    db.write batch, (err) ->
      throw err if err
      if i < totalSize then writeBench() else readBenchStart()

  readBenchStart = ->
    console.log 'Fetching and deserializing 100,000 rows...'
    readBench()

  readBench = ->
    --i
    console.log "i = #{i}" if i % 10000 == 0
    key = "row#{i}"
    db.get key, (err, value) ->
      throw err if err
      value = JSON.parse value
      assert.equal value.index, i
      if i > 0 then readBench() else callback()

  callback = ->
    delta = Date.now() - start;
    console.log 'Completed in %d ms', delta
    db = null
    gc?() # explicit gc if enabled - useful for debugging memleaks

  writeBenchStart()
