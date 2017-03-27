## Compile

### Linux

It would easy to compile it with mex and gdal. First, ensure `gdal` (and its development headers/libs) is installed via apt-get/yum/dnf/pacman/self-compiling. Then call

`$MATLABROOT/bin/mex -v -lgdal mexgdal.c`

The provided makefile assumes MATLAB 2017a and gdal are installed at system default position. Please change them accordingly.

### Windows

Pre-built binrary (Win10 x64, MATLAB 2015a, VC++ 2010, GDAL 2.1.3) is provided. Dependencies:
1. GDAL 2.1.3 (VS2010, x64 build) from GISInternals. A direct download link is [here](http://download.gisinternals.com/sdk/downloads/release-1600-x64-gdal-2-1-3-mapserver-7-0-4/gdal-201-1600-x64-core.msi). 
2. Install. Add the installation dir into PATH system environmental variables. 
3. Add GDAL_DATA=%GDALInstallDir%/gdal_data and PROJ_LIB=%GDALInstallDir%/projlib variable to the system environmental variables
4. (Optional) Verify all dependent DLLs are searchable. You may use [Dependency Walker](http://www.dependencywalker.com)
5. Run `test_gdal.m`

For other version, you should adjust `makefile.win32` and setup C compilers for MATLAB.

### MacOS
Install `gdal` via `homebrew`, then should be same with Linux

## License
NO WARRANTY. Apache License 2.0 if proper.

## TODO
Return spatial reference compatible with Mapping Toolboxes. (Partially *done*. Parse WKT with `prjparse` function)

