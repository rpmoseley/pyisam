cc = meson.get_compiler('c')

isam_incl = include_directories('include')
isam_lib1 = cc.find_library('ifisam', dirs: meson.current_source_dir())
isam_lib2 = cc.find_library('ifisamx', dirs: meson.current_source_dir())

library('pyifisam',
        c_args: '-melf_i386',
        dependencies: [isam_lib1, isam_lib2],
        install_rpath: '$ORIGIN/../lib')
