function z = mexgdal ( input_file, options )
% MEXGDAL:  mex file interface to GDAL library
%
% USAGE: output_arg = mexgdal ( input_file, options );
%
% You shouldn't use mexgdal directly.  Use readgdal.m instead.
%
% PARAMETERS:
% Input:
%     input_file:  a raster file that the GDAL library can read
%     options:  a matlab structure with the following fields:
%
%          band:
%               Optional.  If the input file has multiple bands (e.g. color PNGs have 3), 
%               then you can get a specific band this way.  Specifying this option with a 
%               the numerical value of the band will retrieve that band only.  Otherwise,
%               in the case of a color PNG, for instance, the "z" output would be an 
%               nx x ny x 3 MATLAB image, where nx and ny are the width and height of the
%               image.
%
%          overview:
%              Optional.  If the input file has multiple overviews, 
%              then you can get a specific overview by specifying this option with 
%              the numerical value of the overview you want.  Overview numbers start at 0.  
%              If no overview is specified, then the smallest overview will be retrieved.  
%              If there are no overviews, then you can create them with the gdaladdo 
%              utility (part of the GDAL source distribution).
%          gdal_dump:
%              An integer.  Default is 0.  If 1, then the only action performed is to
%              return the metadata structure.  Otherwise, a raster I/O operation is
%              performed.  If you really want to do this, use gdaldump instead.
%          verbose:
%              Developer use only.  If present and equal to 1, this will trigger a lot of 
%              printfs that say what's going on during the execution of the code.  
%              Default is 0.
%
% Output:
%     output_arg:
%         Usually this is a raster array, but if options.gdal_dump = 1, then the output
%         argument is a structure with metadata.  See gdaldump.m for more information.
%    
% 
