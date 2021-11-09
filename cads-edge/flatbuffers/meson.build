flatc = find_program(['flatc'], required : false)

z_data = custom_target(
    'z_data',
    input : 'z_data.fbs',
		output : 'z_data_generated.h',
    command : [flatc,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

p_config = custom_target(
		'p_config',
    input : 'p_config.fbs',
		output : 'p_config_generated.h',
    command : [flatc,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

cadsSrc += [p_config,z_data]

