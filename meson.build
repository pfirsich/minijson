project('minijson', 'cpp', default_options : ['warning_level=3', 'cpp_std=c++17'])

minijson = static_library('minijson', ['minijson.cpp'])
minijson_dep = declare_dependency(
    include_directories : include_directories('.'),
    link_with : [minijson])

if not meson.is_subproject()
  executable('minijson-test', 'test.cpp', dependencies : [minijson_dep])
endif
