{
	"targets": [
		{
			"target_name": "silk_codec_addon",
			"sources": [
				'<!@(ls -1 src/*.cc)',
				'<!@(ls -1 src/silk/src/*.c)'
			],
			"include_dirs": [
				"<!@(node -p \"require('node-addon-api').include\")",
				"<(module_root_dir)/src",
				"<(module_root_dir)/src/silk/interface",
			],
			'cflags!': [ '-fno-exceptions' ],
			'cflags_cc!': [ '-fno-exceptions' ],
      		'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      		'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS']
		}
	]
}
