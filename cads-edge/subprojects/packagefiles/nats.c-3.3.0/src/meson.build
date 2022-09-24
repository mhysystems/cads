includeDirectories += include_directories('.')

sourceFiles += files(
'asynccb.c',
'buf.c',
'comsock.c',
'conn.c',
'crypto.c',
'hash.c',
'js.c',
'jsm.c',
'kv.c',
'msg.c',
'nats.c',
'natstime.c',
'nkeys.c',
'nuid.c',
'opts.c',
'parser.c',
'pub.c',
'srvpool.c',
'stats.c',
'status.c',
'sub.c',
'timer.c',
'url.c',
'util.c')

if host_machine.system() == 'linux'
  subdir('unix')
endif
