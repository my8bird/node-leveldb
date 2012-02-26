binding = require '../leveldb.node'

###

    An iterator allows sequential and random access to the database.

    Usage:

        var leveldb = require('leveldb');

        leveldb.open('/tmp/test.db', function(err, db) {
          db.iterator(function(err, it) {

            // iterator is initially invalid
            it.first(function(err) {

              // get a key
              it.get('foobar', function(err, val) {

                console.log(val);

              });
            });
          });
        });

###

exports.Iterator = class Iterator

  busy = false
  isValid = false
  keybuf = null
  valbuf = null

  lock = ->
    throw new Error 'Concurrent operations not supported' if busy
    busy = true

  unlock = ->
    throw new Error 'Not locked' unless busy
    busy = false

  afterSeek = (callback) -> (err, valid, key, val) ->
    unlock()
    isValid = valid
    keybuf = key
    valbuf = val
    callback err if callback

  getKey = (options = {}) ->
    unless keybuf
      return null
    else unless options.as_buffer
      keybuf.toString 'utf8'
    else
      keybuf

  getValue = (options = {}) ->
    unless valbuf
      return null
    else unless options.as_buffer
      valbuf.toString 'utf8'
    else
      valbuf


  ###

      Constructor.

      @param {Native} self The underlying iterator object.

  ###

  constructor: (@self) ->


  ###

      Apply a callback over a range.

      The iterator will be positioned at the given key or the first key if
      not given, then the callback will be applied on each record moving
      forward until the iterator is positioned at the limit key or at an
      invalid key. Stops on first error.

      @param {String|Buffer} [startKey] Optional start key (inclusive) from
        which to begin applying the callback. If not given, defaults to the
        first key.
      @param {String|Buffer} [limitKey] Optional limit key (inclusive) at
        which to end applying the callback.
      @param {Object} [options] Optional options.
        @param {Boolean} [options.as_buffer=false] If true, data will be
          returned as a `Buffer`.
      @param {Function} callback The callback to apply to the range.
        @param {Error} error The error value on error, null otherwise.
        @param {String|Buffer} key The key.
        @param {String|Buffer} value The value.

  ###

  forRange: ->

    args = Array.prototype.slice.call arguments

    # required callback
    callback = args.pop()
    throw new Error 'Missing callback' unless callback

    # optional options
    options = args[args.length - 1]
    if typeof options is 'object' and not Buffer.isBuffer options
      args.pop()
    else
      options = {}

    # optional keys
    [ startKey, limitKey ] = args

    limit = limitKey.toString 'binary' if limitKey

    next = (err) =>
      return callback err if err
      if isValid
        callback null, getKey(options), getValue(options)
        if not limit or limit isnt keybuf.toString 'binary'
          @next next

    if startKey
      @seek startKey, next
    else
      @first next


  ###

      True if the iterator is positioned at a valid key.

  ###

  valid: -> isValid


  ###

      Position the iterator at a key.

      @param {String} key The key at which to position the iterator.
      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  seek: (key, callback) ->
    key = new Buffer key unless Buffer.isBuffer key
    lock()
    @self.seek key, afterSeek callback


  ###

      Position the iterator at the first key.

      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  first: (callback) ->
    lock()
    @self.first afterSeek callback


  ###

      Position the iterator at the last key.

      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  last: (callback) ->
    lock()
    @self.last afterSeek callback


  ###

      Advance the iterator to the next key.

      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  next: (callback) ->
    throw new Error 'Illegal state' unless isValid
    lock()
    @self.next afterSeek callback


  ###

      Advance the iterator to the previous key.

      @param {Function} [callback] Optional callback.
        @param {Error} error The error value on error, null otherwise.

  ###

  prev: (callback) ->
    throw new Error 'Illegal state' unless isValid
    lock()
    @self.prev afterSeek callback


  ###

      Get the key at the current iterator position.

      @param {Object} [options] Optional options.
        @param {Boolean} [options.as_buffer=false] If true, data will be
          returned as a `Buffer`.
      @param {Function} callback The callback function.
        @param {Error} error The error value on error, null otherwise.
        @param {String|Buffer} key The key if successful.

  ###

  key: (options, callback) ->
    throw new Error 'Illegal state' unless isValid

    # optional options
    if typeof options is 'function'
      callback = options
      options = null

    throw new Error 'Missing callback' unless callback

    callback null, getKey options


  ###

      Get the value at the current iterator position.

      @param {Object} [options] Optional options.
        @param {Boolean} [options.as_buffer=false] If true, data will be
          returned as a `Buffer`.
      @param {Function} callback The callback function.
        @param {Error} error The error value on error, null otherwise.
        @param {String|Buffer} value The value if successful.

  ###

  value: (options, callback) ->

    # optional options
    if typeof options is 'function'
      callback = options
      options = null

    callback null, getValue options


  ###

      Get the key and value at the current iterator position.

      @param {Object} [options] Optional options.
        @param {Boolean} [options.as_buffer=false] If true, data will be
          returned as a `Buffer`.
      @param {Function} callback The callback function.
        @param {Error} error The error value on error, null otherwise.
        @param {String|Buffer} key The key if successful.
        @param {String|Buffer} value The value if successful.

  ###

  current: (options, callback) ->

    # optional options
    if typeof options is 'function'
      callback = options
      options = null

    throw new Error 'Missing callback' unless callback

    callback null, getKey(options), getValue(options)
