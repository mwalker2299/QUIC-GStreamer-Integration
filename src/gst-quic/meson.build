project('gst-quic', 'c', version : '1.16.2', license : 'LGPL')

plugins_install_dir = '/usr/lib/x86_64-linux-gnu/gstreamer-1.0'
cc = meson.get_compiler('c')

gst_version = meson.project_version()

api_version = '1.0'

gst_dep = dependency('gstreamer-1.0', version : '>=1.16',
    required : true, fallback : ['gstreamer', 'gst_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : '>=1.16',
  fallback : ['gstreamer', 'gst_base_dep'])


subdir('plugins')
