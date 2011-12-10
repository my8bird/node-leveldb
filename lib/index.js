lib = require('../build/leveldb.node');

for (i in lib) {
   exports[i] = lib[i];
}

