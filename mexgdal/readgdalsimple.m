function [varargout] = readgdalsimple ( gdal_file )
% READGDALSIMPLE:  Assumes the most basic defaults in reading a gdal image.
%
% If the image is big (and geotiffs are often quite big), then this
% m-file may take a while to run.  
%
% If there is more than one band, then this m-file reads each raster
% band, ignoring overviews.
%
% This routine will not work if the bands have differing resolutions, 
% which is rare, but can happen.  In that case, you should do a bit of
% programming and use readgdalband instead.  Check the documentation
% in the doc/html subdirectory.
%
% USAGE:  [x, y, z] = readgdalsimple ( raster_file );
%
% PARAMETERS:
% Inputs:
%     raster_file:
%         Some raster file.  It is hopefully supported by the GDAL library.
%
% Output:
%     x, y:
%         Coordinates arrays at which the data is defined.  See explanation of "grid"
%         input field.
%     z:  
%         raster data read from the GDAL raster file.  Type is uint8.  
%         If there is more than one band, then z will have three 
%         dimensions, the third being the band.
%         
% See also READGDALBAND, GDALDUMP, IMAGE


% Before we do anything, check if the user doesn't expect too much output:
if (nargout == 0) || (nargout == 2) || (nargout > 3)
    error ( '%s:  This function requires either one or three outputs.\n', mfilename);
end

input_options = struct ( 'band', 1, ...
	'verbose', 0, ...
	'grid', 0, ...
	'xorigin', 0, ...
	'yorigin', 0, ...
	'xextend', 1, ...
	'yextend', 1, ...
	'xout', 1, ...
	'yout', 1 );


x = [];
y = [];

metadata = gdaldump ( gdal_file );


gdal_options = mexgdal_validate_input_options ( input_options, metadata );





num_bands = metadata.RasterCount;

%
% Assume that the type will be that of the first band.
if strcmp ( metadata.Band(1).DataType, 'Byte' )
	z = uint8(zeros( metadata.RasterYSize, metadata.RasterXSize, num_bands ));
else
	z = double(zeros( metadata.RasterYSize, metadata.RasterXSize, num_bands ));
end

%
% Set to the default size.
input_options.xextend = metadata.RasterXSize;
input_options.yextend = metadata.RasterYSize;
input_options.xout = metadata.RasterXSize;
input_options.yout = metadata.RasterYSize;

if num_bands == 1
	gdal_options = mexgdal_validate_input_options ( input_options, metadata );
	z = mexgdal ( gdal_file, gdal_options );
else
	for j = 1:metadata.RasterCount

		input_options.band = j;

		gdal_options = mexgdal_validate_input_options ( input_options, metadata );
		z(:,:,j) = mexgdal ( gdal_file, gdal_options );

	end
end


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



if ~isempty(metadata.GeoTransform)
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




