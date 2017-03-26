function [varargout] = readgdalband ( gdal_file, input_options )
% READGDALBAND:  reads a raster data format supported by GDAL library, single band
%
% USAGE:  z = readgdalband ( raster_file, input_options );
% USAGE:  [x, y, z] = readgdalband ( raster_file, input_options );
%
% PARAMETERS:
% Inputs:
%     raster_file:
%         Some raster file.  It is hopefully supported by the GDAL library.
%     input_options:
%         A structure with several optional arguments.  If this structure is not
%         provided, then default assumptions are followed.  Allowed fields include 
%
%         band:
%             Optional.  If the input file has multiple bands (e.g. color PNGs have 3), 
%             then you can get a specific band this way.  Specifying this option with a 
%             the numerical value of the band will retrieve that band only.  Otherwise,
%             in the case of a color PNG, for instance, the "z" output would be an 
%             nx x ny x 3 MATLAB image, where nx and ny are the width and height of the
%             image.
%        
%             If the band number is not supplied, then it is set to 1.
%
%             Bands are designated starting with 1.  There is no band 0.
%      
%         grid:
%             Optional.  If present and equal to 1, then the output arguments x and y
%             are defined to be full grids equal in dimension to that of the z output
%             argument.  Otherwise the working assumption is that you intend to use
%             the output as you would a matlab image, so x and y will be returned as 
%             [upper-left lower-right] coordinates.  See IMAGE.
%         overview:
%             Optional.  If the input file has multiple overviews, 
%             then you can get a specific overview by specifying this option with 
%             the numerical value of the overview you want.  Overview numbers start at 0.  
%             If no overview is specified, then the smallest overview will be retrieved.  
%             If the overview flag is specified to be 'largest' then the largest overview 
%             will be retrieved (usually overview #0).  If there are no overviews, then 
%             you can create them with the gdaladdo utility (part of the GDAL source 
%             distribution).
%         gdal_dump:
%             An integer.  Default is 0.  If 1, then the only action performed is to
%             return the metadata structure.  Otherwise, a raster I/O operation is
%             performed.  If you really want to do this, use gdaldump instead.
%         verbose:
%             Developer use only.  If present and equal to 1, this will trigger a lot of 
%             printfs that say what's going on during the execution of the code.  
%             Default is 0.
%         xOrigin, yOrigin:
%             Optional integers. Coordinates of the upper left cell of output within the original
%             raster file. Both values default to 0 (upper left corner of the raster). 
%         xExtend, yExtend:
%             Optional integers. Size of the partial image to be read in collumns and rows. 
%             Defaults to maximum size (lower right corner of the raster).
%         xOut, yOut:
%             Optional integers. The scaled output size. xOut defaults to
%             xExtend. yOut defaults to yExtend.
%
% Output:
%     x, y:
%         Coordinates arrays at which the data is defined.  See explanation of "grid"
%         input field.
%     z:  
%         raster data read from the GDAL raster file
%         
% See also GDALDUMP, IMAGE

% Improvements *Origin, *Extend, *Out: David Balder (http://www.science.uva.nl/~dbalder)
% x = colls, y = rows.

% Before we do anything, check if the user doesn't expect too much output:
if (nargout == 0) || (nargout == 2) || (nargout > 3)
    error ( '%s:  This function requires either one or three outputs.\n', mfilename);
end


x = [];
y = [];

metadata = gdaldump ( gdal_file );


%
% the input options field must at least have the band field.
if ~isfield ( input_options, 'band' )
	input_options.band = 1;
end


gdal_options = mexgdal_validate_input_options ( input_options, metadata );








z = mexgdal ( gdal_file, gdal_options );

%
% Was there a no data value?
if isfinite ( metadata.Band(1).NoDataValue )
    z(z==metadata.Band(1).NoDataValue) = NaN;
%     z(ind) = NaN;
end

if nargout == 1
    varargout{1} = z;
    return
end
if nargout == 2
    varargout{1} = z;
    varargout{2} = metadata;
    return
end

if length(metadata.GeoTransform) ~= 0

	% Construct x and y
	x0 = metadata.GeoTransform(1);
	xinc = metadata.GeoTransform(2);
	y0 = metadata.GeoTransform(4);
	yinc = metadata.GeoTransform(6);
	% Now this is is becoming a bit tricky because of the origin, etc. It
	% should work, though...
	xOut0 = x0 + gdal_options.xorigin * xinc;
	yOut0 = y0 + gdal_options.yorigin * yinc;
	xOutExt = xOut0 + (gdal_options.xextend - 1) * xinc;
	yOutExt = yOut0 + (gdal_options.yextend - 1) * yinc;
    
	if gdal_options.grid
		xOutInc = (xOutExt - xOut0 ) / (gdal_options.xout - 1);
		yOutInc = (yOutExt - yOut0 ) / (gdal_options.yout - 1);
		X = xOut0:xOutInc:xOutExt;
		Y = yOut0:yOutInc:yOutExt;
		[x,y] = meshgrid(X,Y);
	else
		x = [xOut0 xOutExt];
		y = [yOut0 yOutExt];
	end
end

if ( nargout == 3 )
	varargout{1} = x;
	varargout{2} = y;
	varargout{3} = z;
	return
end

if ( nargout == 4 )
	varargout{1} = x;
	varargout{2} = y;
	varargout{3} = z;
	varargout{4} = metadata;
	return
end



