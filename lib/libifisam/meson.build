# Include the appropriate architecture subdirectory
message('SYS: ' + host_machine.system()) 
cpu_family = build_machine.cpu_family()
if cpu_family == 'i686'
  subdir('32')
elif cpu_family == 'x86_64'
  subdir('64')
else
  error('Unsupported build machine cpu')
endif
#isam_incl = arch_bits / 'include'
#isam_libs = [arch_bits / 'libifisam.so', arch_bits / 'libifisamx.so']
#isam_pylib = arch_bits / 'libpyisam.so'
#isam_dep = declare_dependency(link_with : isam_libs, include_directories : isam_incl)
