{spawn} = require 'child_process'
path = require 'path'

exec = (command, callback) ->
  args = command.split /\s/
  command = args.shift()
  proc = spawn command, args
  proc.stdout.on 'data', (chunk) -> process.stdout.write chunk
  proc.stderr.on 'data', (chunk) -> process.stderr.write chunk
  proc.on 'exit', (code, signal) ->
    throw "#{command} exited with code #{code}" if code
    throw "#{command} received signal #{signal}" if signal
    callback?()

configure = (callback) ->
  exec 'node-waf configure', callback

compileBinding = (callback) ->
  exec 'node-waf build', callback

compileCoffee = (callback) ->
  exec 'coffee --bare --compile --output lib src', callback

clean = (callback) ->
  exec 'node-waf clean', callback

task 'build', 'Configure and build native binding and coffeescript', ->
  unless path.existsSync "#{__dirname}/deps/leveldb/build_config.mk"
    configure -> compileBinding -> compileCoffee()
  else
    compileBinding -> compileCoffee()

task 'build:configure', 'Configure files for building', ->
  configure()

task 'build:binding', 'Compile native binding', ->
  compileBinding()

task 'build:coffee', 'Compile coffee to js', ->
  compileCoffee()

task 'clean', 'Remove compiled files', ->
  clean()
