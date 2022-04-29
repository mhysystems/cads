z_data = custom_target(
    'z_data',
    input : 'z_data.fbs',
		output : 'z_data_generated.h',
    command : [flatc,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

z_data_csharp = custom_target(
    'z_data_csharp',
    input : 'z_data.fbs',
		output : 'z_data_generated.cs',
    command : [flatc,'-o','@OUTDIR@','--csharp', '@INPUT@'],
)

csharpGen = generator(
  flatc,
  output : '@BASENAME@.cs',
  arguments : ['-o','@OUTDIR@','--csharp','--csharp', '@INPUT@']
)

p_config = custom_target(
		'p_config',
    input : 'p_config.fbs',
		output : 'p_config_generated.h',
    command : [flatc,'-o','@OUTDIR@','--cpp', '@INPUT@'],
)

csharpGen.process('z_data.fbs')
cadsSrc += [p_config,z_data]

