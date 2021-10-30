plugin_c_args = ['-DHAVE_CONFIG_H','-D_GNU_SOURCE']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-template-plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer template Plug-ins')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://gstreamer.freedesktop.org')
configure_file(output : 'config.h', configuration : cdata)


lsquic_dep = cc.find_library('lsquic', dirs: '/home/matt/Documents/lsquic/src/liblsquic/', required: true)
inc = include_directories('/home/matt/Documents/lsquic/include')



# The quic Plugin
 gstquic_sources = [
  'gstquicsrc.c',
  'gstquicsink.c',
  'gstquic.c',
  'gstquicutils.c'
  ]

gstquic = library('gstquic',
  gstquic_sources,
  c_args: plugin_c_args,
  include_directories : inc,
  dependencies : [gst_dep, gstbase_dep, lsquic_dep],
  install : true,
  install_dir : plugins_install_dir,
)
