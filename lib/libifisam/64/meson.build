cc = meson.get_compiler('c')

libisam_dep = declare_dependency(
  dependencies: [
    cc.find_library('ifisam', dirs: meson.current_source_dir()),
    cc.find_library('ifisamx', dirs: meson.current_source_dir())
  ],
  include_directories: include_directories('.')
)
meson.override_dependency('libisam', libisam_dep)
