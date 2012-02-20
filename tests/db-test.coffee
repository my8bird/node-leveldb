assert  = require 'assert'
leveldb = require '../lib'
path    = require 'path'


describe 'db', ->
  db = null
  filename = "#{__dirname}/../tmp/db-test-file"

  itShouldBehaveLikeAKeyValueStore = (key, val) ->

    it 'should open database', (done) ->
      leveldb.open filename, create_if_missing: true, paranoid_checks: true, (err, handle) ->
        assert.ifError err
        db = handle
        done()

    it 'should put key/value pair', (done) ->
      db.put key, val, done

    it 'should get key/value pair', (done) ->
      db.get key, (err, result) ->
        assert.ifError err
        assert.equal val.toString(), result
        done()

    it 'should delete key', (done) ->
      db.del key, done

    it 'should not get key/value pair', (done) ->
      db.get key, (err, result) ->
        assert.ifError err
        assert.equal undefined, result
        done()

    it 'should close database', (done) ->
      db = null
      process.nextTick done

    it 'should repair database', (done) ->
      leveldb.repair filename, (err) ->
        assert.ifError err
        assert path.existsSync filename
        done()

    it 'should destroy database', (done) ->
      leveldb.destroy filename, done

  describe 'admin', ->

    it 'should open database', (done) ->
      leveldb.open filename, create_if_missing: true, (err, handle) ->
        assert.ifError err
        db = handle
        done()

    it 'should get property', (done) ->
      db.property 'leveldb.stats', (err, value) ->
        assert.ifError err
        assert value
        db.property '', (err, value) ->
          assert.ifError value
          done err, value

    it 'should get property (sync)', ->
      assert db.propertySync 'leveldb.stats'
      assert.equal undefined, db.propertySync ''

    it 'should get no approximate size', ->
      assert.equal 0, db.approximateSizesSync []

    it 'should get one approximate size', ->
      assert.equal 0, db.approximateSizesSync '0', '1'

    it 'should put values', (done) ->
      batch = db.batch()
      batch.put "#{i}", 'Hello World!' for i in [0..10000]
      batch.put '100', 'Goodbye World!'
      batch.write sync: true, done

    it 'should close database', (done) ->
      db = null
      process.nextTick done

    it 'should open database', (done) ->
      leveldb.open filename, (err, handle) ->
        assert.ifError err
        db = handle
        done()

    it 'should get value', (done) ->
      db.get '100', (err, value) ->
        assert.equal 'Goodbye World!', value
        done()

    it 'should get approximate sizes (sync)', ->
      sizes = db.approximateSizesSync '100', '200'
      assert Array.isArray sizes
      assert.equal 1, sizes.length
      assert.equal 'number', typeof sizes[0]

    it 'should get approximate sizes (sync)', ->
      sizes = db.approximateSizesSync [[ '100', '150' ], [ '150', '200' ]]
      assert Array.isArray sizes
      assert.equal 2, sizes.length
      assert.equal 'number', typeof sizes[0]
      assert.equal 'number', typeof sizes[1]

    it 'should get approximate sizes (sync)', ->
      sizes = db.approximateSizesSync [ '100', '150' ], [ '150', '200' ], [ '200', '250' ]
      assert Array.isArray sizes
      assert.equal 3, sizes.length
      assert.equal 'number', typeof sizes[0]
      assert.equal 'number', typeof sizes[1]
      assert.equal 'number', typeof sizes[2]

    it 'should get approximate sizes (async)', (done) ->
      db.approximateSizes '100', '200', (err, sizes) ->
        assert.ifError err
        assert Array.isArray sizes
        assert.equal 1, sizes.length
        assert.equal 'number', typeof sizes[0]
        done()

    it 'should get approximate sizes (async)', (done) ->
      db.approximateSizes [[ '100', '150' ], [ '150', '200' ]], (err, sizes) ->
        assert.ifError err
        assert Array.isArray sizes
        assert.equal 2, sizes.length
        assert.equal 'number', typeof sizes[0]
        assert.equal 'number', typeof sizes[1]
        done()

    it 'should get approximate sizes (async)', (done) ->
      db.approximateSizes [ '100', '150' ], [ '150', '200' ], [ '200', '250' ], (err, sizes) ->
        assert.ifError err
        assert Array.isArray sizes
        assert.equal 3, sizes.length
        assert.equal 'number', typeof sizes[0]
        assert.equal 'number', typeof sizes[1]
        assert.equal 'number', typeof sizes[2]
        done()

    it 'should close database', (done) ->
      db = null
      process.nextTick done

  describe 'with ascii values', ->
    itShouldBehaveLikeAKeyValueStore "Hello", "World"

  describe 'with buffer values', ->
    key = new Buffer [1,9,9,9]
    val = new Buffer [1,2,3,4]
    itShouldBehaveLikeAKeyValueStore key, val
