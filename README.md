[![Build Status](https://secure.travis-ci.org/my8bird/node-leveldb.png)](http://travis-ci.org/my8bird/node-leveldb)

# Node-LevelDB

The leveldb library provides a persistent key value store. Keys and values are arbitrary byte arrays. The keys are ordered within the key value store according to a user-specified comparator function.

This project is node bindings to this excellent library so that node programs can either implement their own custom databases or simply use it directly as a fast, simple key/value store.

While implementing nStore, I realized there are a couple things that V8 and node are not very good at.  They are heavy disk I/O and massive objects with either millions of keys or millions of sub objects.

Since LevelDB provides good primitives like MVCC and binary support (It was designed to back IndexDB in the Chrome browser), then it can be used as a base to implement things like CouchDB.

## TODO

 * get the build working for 0.7

leveldb.open("path/to/my/db", { create_if_missing: true }, onOpen);

function onOpen(err, db) {
  var key = "mykey";
  db.put(key, "My Value!", function(err) {
    db.getSync(key, function(err, value) {
      console.dir(value); // prints: My Value!
      db.del(key);
    });
  });
}

The API is meant to be an almost one-to-one mapping to the underlying C++ library.  Essentially it's a key/value store, but has some extra nice things like sorted keys, snapshots and iterators.

### LevelDB

This is the main Class and probably the only one that a user needs to create manually.

    var DB = require('leveldb.node').DB;
    var db = new DB;
    db.open({create_if_missing: true}, "path/to/my/db");
    var key = new Buffer("myKey");
    db.put({}, key, new Buffer("My Value!"));
    var value = db.get({}, key);
    console.dir(value);
    db.del({}, key);
    db.close();

I'll document more as this stabilizes.  In the mean-time, check out the demo scripts I use for testing.
<https://github.com/my8bird/node-leveldb/blob/master/demo>

## Compiling

I've only tested on my Ubuntu laptop, but it compiles cleanly using only the wscript file.  To compile simple type.

    node-waf configure build

Then to test, run the demo script

    node demo/demo.js

The leveldb library is bundled in the deps folder and is compiled in staticly.  The resulting node addon is about 4mb unstripped.  If you want to make it smaller, I've had success using the `strip` command on it which shrinks it down to about 300kb.

    strip build/default/leveldb.node

## Contributing

Since I am not experienced in the world of C++ and am still learning, I welcome contributions.  Mainly what I need right now is someone with experience to review my code and tell me things I'm doing stupid.  Maybe add a C++ best pratices note to this document.  Also once the initial blocking version of the library is done, I'll need to add async ability to all the functions that do disk I/O.

Currently Randall Leeds (@tilgovi and one of the CouchDB commiters) has expressed interest in helping and already has commit rights to the project.

## Roadmap

This is the short-term and long roadmap to track progress.

- Implement just enough to make a simple key/value store. (100% done)
- Implement bulk writes
- Implement Iterators
- Implement Snapstops
- Implement getProperty and getApproximateSizes
- ...
- Add async versions of all functions that use disk I/O
- ...
- Hand off project to a loyal maintainer and start my next adventure.

## Contributors

```
   161  Michael Phan-Ba
    39  Tim Caswell
    22  Nathan Landis
    19  gabor@google.com
    15  jorlow@chromium.org
    12  dgrogan@chromium.org
    12  Stefan Thomas
     9  Carter Thaxton
     9  Randall Leeds
     6  Damon Oehlman
     5  Hans Wennborg
     4  shinuza
     3  hans@chromium.org
     2  Sanjay Ghemawat
     1  (no author)
     1  Gabor Cselle
```
