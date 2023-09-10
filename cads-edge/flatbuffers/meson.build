# Source is shared with gui project, hence copied as it seems non trivial to build
# progjects in extrnal directories for build systems used in cads
#copy = find_program('cp')
#run_command(copy,join_paths(meson.source_root(),'..','flatbuffers-schema','z_data.fbs'),join_paths(meson.source_root(),'flatbuffers','z_data.fbs'))
#run_command(copy,join_paths(meson.source_root(),'..','flatbuffers-schema','plot_data.fbs'),join_paths(meson.source_root(),'flatbuffers','plot_data.fbs'))
#run_command(copy,join_paths(meson.source_root(),'..','flatbuffers-schema','cads_msg.fbs'),join_paths(meson.source_root(),'flatbuffers','cads_msg.fbs'))

z_data = custom_target(
    'z_data',
    input : 'z_data.fbs',
		output : 'z_data_generated.h',
    command : [flatc_exe,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

plot_data = custom_target(
    'plot_data',
    input : 'plot_data.fbs',
		output : 'plot_data_generated.h',
    command : [flatc_exe,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

cads_msg = custom_target(
    'cads_msg',
    input : 'cads_msg.fbs',
		output : 'cads_msg_generated.h',
    command : [flatc_exe,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

cadsSrc += [z_data,plot_data,cads_msg]

