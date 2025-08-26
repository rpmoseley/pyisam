# Build the binaries that are part of vbisam
vbisam_bin_incl = vbisam_incl + include_directories(['..'])
vbcheck = executable('vbcheck', files(['vbcheck.c', 'vbisam.h']), include_directories: vbisam_bin_incl, dependencies: [vbisam])
vbrecover = executable('vbrecover', files(['vbrecover.c', 'vbisam.h']), include_directories: vbisam_bin_incl, dependencies: [vbisam])
