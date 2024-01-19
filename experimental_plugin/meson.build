
nghttp2 = dependency('libnghttp2', version: '>=1.52.0', required : false)
if not nghttp2.found()
  cmake = import('cmake')
  opt_var = cmake.subproject_options()
  opt_var.add_cmake_defines({
    'ENABLE_LIB_ONLY': true,
    'ENABLE_STATIC_LIB': true
  })
  nghttp2_ex = cmake.subproject('nghttp2', options: opt_var)
  nghttp2 = nghttp2_ex.dependency('nghttp2')
endif


pluginname='experimental_plugin'
plugin_sources = [
    'src/experimental_plugin.cpp',
    '../src/json_utils.cpp',
    '../src/error_messages.cpp',
    '../src/dbus_singleton.cpp'
]


core_includes = include_directories(['.',
'../subprojects/bmcweb/redfish-core/include',
'../subprojects/bmcweb/http',
'../subprojects/bmcweb/',
'../subprojects/bmcweb/include',
'..',
'../plugin-ifaces'])


plugin_deps = [
json_dep,boost_dep,openssl_dep
]



library(pluginname, 
    sources : plugin_sources,
    include_directories : core_includes,
    dependencies : [plugin_deps,nghttp2],
    cpp_args : WEB_ARGS,
    link_with : [])
    


