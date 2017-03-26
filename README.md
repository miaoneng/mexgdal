## Compile

### Linux

It would easy to compile it with mex and gdal. First, ensure `gdal` is installed via apt-get/yum/dnf/pacman/self-compiling. Then call

`$MATLABROOT/bin/mex -v -lgdal mexgdal.c`

The provided makefile assumes MATLAB 2017a and gdal are installed at system default position. Please change them accordingly.

### Windows

Didn't try. But I am pretty sure it works with MSYS2 as long as all related packages are installed. Tutorials and pre-built binary would be provided latter.

### MacOS
Install `gdal` via `homebrew`, then should be same with Linux

## License
NO WARRANTY. Apache License 2.0 if proper.

## TODO
Return spatial reference compatible with Mapping Toolboxes.

