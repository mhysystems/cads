z_data = custom_target(
    'z_data',
    input : 'z_data.fbs',
		output : 'z_data_generated.h',
    command : [flatc,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

cadsSrc += [z_data]

