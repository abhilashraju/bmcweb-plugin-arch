project(
  'web',
  'cpp',
  version: '0.1',
  meson_version: '>=0.58.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
  ])
cxx = meson.get_compiler('cpp')
WEB_ARGS=['-DBMCWEB_ENABLE_SSL','-DBMCWEB_INSECURE_DISABLE_AUTHX','-DBOOST_URL_NO_SOURCE_LOCATION','-DBMCWEB_INSECURE_DISABLE_AUTHX']

openssl_dep = dependency('openssl', version: '>=1.1.1')
json_dep = dependency('nlohmann_json')
#bmcweb=subproject('bmcweb')


boost_dep = dependency(
  'boost',
  modules: [
    'url',
    ],
  #version : '>=1.83.0',
  required : true,
  include_type: 'system'
)

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

pam = cxx.find_library('pam', required: true)
atomic =  cxx.find_library('atomic', required: true)

web_dependencies = [nghttp2,pam,atomic]

sdbusplus = dependency('sdbusplus', required : false, include_type: 'system')
if not sdbusplus.found()
  sdbusplus_proj = subproject('sdbusplus', required: true)
  sdbusplus = sdbusplus_proj.get_variable('sdbusplus_dep')
  sdbusplus = sdbusplus.as_system('system')
endif
web_dependencies += sdbusplus
web_includes = include_directories(['include','subprojects/bmcweb/',
'subprojects/bmcweb/http',
'subprojects/bmcweb/include',
'subprojects/bmcweb/redfish-core/include',
'subprojects/bmcweb/redfish-core/lib',
'.'
])

subdir('redfish_event')
subdir('dump_offload')
subdir('experimental_plugin')
executable(
  'web',
  [
    'src/web.cpp',
    'src/dbus_singleton.cpp',
    'src/error_messages.cpp',
    'src/json_utils.cpp',
    'src/time_utils.cpp',
    'src/ossl_random.cpp',
    'src/json_html_serializer.cpp',
    'src/registries.cpp',
    
    

  ],
  include_directories: web_includes,
  dependencies:[boost_dep,json_dep,openssl_dep,web_dependencies],
  install: true,
  install_dir: get_option('bindir'),cpp_args : WEB_ARGS)