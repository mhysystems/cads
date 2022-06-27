# Source is shared with gui project, hence copied as it seems non trivial to build
# progjects in extrnal directories for build systems used in cads
copy = find_program('cp')
run_command(copy,join_paths(meson.source_root(),'..','flatbuffers','z_data.fbs'),join_paths(meson.source_root(),'flatbuffers','z_data.fbs'))

z_data = custom_target(
    'z_data',
    input : 'z_data.fbs',
		output : 'z_data_generated.h',
    command : [flatc,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

cadsSrc += [z_data]

