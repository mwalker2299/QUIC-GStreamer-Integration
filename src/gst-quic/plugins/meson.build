plugin_c_args = ['-DHAVE_CONFIG_H','-D_GNU_SOURCE']

cdata = configuration_data()
cdata.set_quoted('PACKAGE_VERSION', gst_version)
cdata.set_quoted('PACKAGE', 'gst-template-plugin')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('GST_PACKAGE_NAME', 'GStreamer template Plug-ins')
cdata.set_quoted('GST_PACKAGE_ORIGIN', 'https://gstreamer.freedesktop.org')
configure_file(output : 'config.h', configuration : cdata)

lsquic_dep = cc.find_library('lsquic', dirs: meson.current_source_dir() + '/../../../submodules/lsquic/src/liblsquic/', required: true)
ssl_dep    = cc.find_library('ssl', dirs: meson.current_source_dir() + '/../../../submodules/boringssl/ssl/', required:true)
crypto_dep = cc.find_library('crypto', dirs: meson.current_source_dir() + '/../../../submodules/boringssl/crypto/', required:true)

lsquic_inc    = include_directories('../../../submodules/lsquic/include')
boringssl_inc = include_directories('../../../submodules/boringssl/include')
# The quic Plugin
 gstquic_sources = [
  'gstquicsrcss.c',
  'gstquicsinkss.c',
  'gstquicsrcpps.c',
  'gstquicsinkpps.c',
  'gstquicsrcgop.c',
  'gstquicsinkgop.c',
  'gstquicsrcfps.c',
  'gstquicsinkfps.c',
  'gstquic.c',
  'gstquicutils.c',
  'gsttcpsink.c',
  'gsttcpsrc.c'
  ]

gstquic = library('gstquic',
  gstquic_sources,
  c_args: plugin_c_args,
  include_directories : [lsquic_inc, boringssl_inc],
  dependencies : [gst_dep, gstbase_dep, lsquic_dep, ssl_dep, crypto_dep],
  install : true,
  install_dir : plugins_install_dir,
)
