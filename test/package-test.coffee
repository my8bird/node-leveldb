assert  = require 'assert'
fs      = require 'fs'
leveldb = require '../lib'


describe 'package', ->
  pkg = JSON.parse fs.readFileSync __dirname + '/../package.json', 'utf8'
  bindingVersion = '1.5'

  it 'should have version', ->
    assert.equal pkg.version, leveldb.version

  it 'should have binding version', () ->
    assert.equal leveldb.bindingVersion, bindingVersion

