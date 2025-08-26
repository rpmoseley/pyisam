# Include the appropriate architecture subdirectory
cpu_family = build_machine.cpu_family()
if cpu_family == 'i686'
  subdir('32')
elif cpu_family == 'x86_64'
  subdir('64')
else
  error('Unsupported build machine cpu')
endif
