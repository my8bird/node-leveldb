{
  "targets": [
    {
      "target_name": "leveldb",
      "sources": [
        "src/cpp/batch.cc",
        "src/cpp/batch.h",
        "src/cpp/binding.cc",
        "src/cpp/comparator.cc",
        "src/cpp/comparator.h",
        "src/cpp/handle.cc",
        "src/cpp/handle.h",
        "src/cpp/helpers.h",
        "src/cpp/iterator.cc",
        "src/cpp/iterator.h",
        "src/cpp/node_async_shim.h",
        "src/cpp/options.h"
      ],
      "dependencies": [
        'deps/leveldb/leveldb.gyp:leveldb'
      ]
    }
  ]
}
