function gdal_options = mexgdal_validate_input_options ( input_options, metadata )
% MEXGDAL_VALIDATE_INPUT_OPTIONS: validates the input_options structure
%
% Any options that are not supplied in the input_options structure are filled in
% with reasonable defaults.
%

%
% Go thru the input parameters.  Verify that they are valid.
% If so, move them over to the options that are to be passed
% into mexgdal.
%
% The default is to read all of the first band, and make the output
% the same size as the first raster band in the file.
gdal_options = struct ( 'band', 1, ...
	'verbose', 0, ...
	'grid', 0, ...
	'xorigin', 0, ...
	'yorigin', 0, ...
	'xextend', metadata.RasterXSize, ...
	'yextend', metadata.RasterYSize, ...
	'xout', metadata.RasterXSize, ...
	'yout', metadata.RasterYSize );



%
% Replace the default options with user-specified options if called for.
if nargin > 1
	the_field_names = fieldnames ( input_options );
	for j = 1:length(the_field_names)

		key = the_field_names{j};
		value = input_options.(key);	  

		switch ( lower(key) )
			case 'band'
				if ~isnumeric(value) || (length(value) ~= 1)
					error ( '%s:  option band must be a scalar.\n', mfilename, key );
				end

				%
				% The band number must be greater than zero.
				if ( value < 1 )
					error ( '%s:  option band number must be greater than zero.\n', mfilename );
				end
				
				gdal_options.band = value;
				
			case 'overview'
				switch class(value)
					case 'double'
						if length(value) ~= 1 
							error ( '%s:  option overview must be a scalar.\n', mfilename, key );
						end
						gdal_options.overview = value;
					otherwise
						error ( '%s:  option overview must be numeric.\n', mfilename );
				end
				
			case { 'verbose' }
				gdal_options.verbose = double(value(1));



			case { 'grid' }
				% If values is 1 or true, we return a full grid, otherwise
				% we return corner coordinates.
				if value 
					gdal_options.grid = true;
				else
					gdal_options.grid = false;
				end



			case { 'xorigin' }
				if (value < 0) || (value > metadata.RasterXSize) || (length(value) ~= 1)
					error ( '%s: Option xOrigin should be a positive integer \n and 0 <= xorigin < %d.\n' , mfilename, metadata.RasterXSize);
				end
				gdal_options.xorigin = value;
			case { 'yorigin' }
				if (value < 0) || (value > metadata.RasterYSize) || (length(value) ~= 1)
					error ( '%s: Option yOrigin should be a positive integer \n and 0 <= yorigin < %d.\n', mfilename, metadata.RasterYSize);
				end
				gdal_options.yorigin = value;
			case { 'xextend' }
				if (value <= 0) || (length(value) ~= 1)
					error ( '%s: Option xExtend should be a positive integer \n and 1 <= xExtend <= %d.\n' , mfilename, metadata.RasterXSize);
				end
				gdal_options.xextend = value;
			case { 'yextend' }
				if (value <= 0) || (length(value) ~= 1)
					error ( '%s: Option yExtend should be a positive integer \n and 1 <= yExtend <= %d.\n' , mfilename, metadata.RasterYSize);
				end
				gdal_options.yextend = value;
			case { 'xout' }
				xout = input_options.xout;

				if (xout <= 0) || (length(xout) ~= 1)
					error ( '%s: Option xOut should be a positive integer.\n', mfilename);
				end

				gdal_options.xout = xout;


			case { 'yout' }
				yout = input_options.yout;

				if (yout <= 0) || (length(yout) ~= 1)
					error ( '%s: Option yOut should be a positive integer.\n', mfilename);
				end

				gdal_options.yout = yout;

				
			otherwise
				error ( '%s:  unknown field ''%s''\n', mfilename, key );
		end
	end
end





%
% Another check. We dont know the order of input, thus has to be done after
% all input has been processed:
% x:
% If only an xorigin was set and xextend is the default:
if (gdal_options.xorigin ~= 0) && (gdal_options.xextend == metadata.RasterXSize)
	gdal_options.xextend = metadata.RasterXSize - gdal_options.xorigin;
end

if (gdal_options.xorigin + gdal_options.xextend > metadata.RasterXSize)
	error ( '%s: xOrigin (%d) + xExtend (%d) cannot be larger then %d.\n', mfilename, gdal_options.xorigin, gdal_options.xextend, metadata.RasterXSize);
end
% y:
if (gdal_options.yorigin ~= 0) && (gdal_options.yextend == metadata.RasterYSize)
	gdal_options.yextend = metadata.RasterYSize - gdal_options.yorigin;
end
if (gdal_options.yorigin + gdal_options.yextend > metadata.RasterYSize)
	error ( '%s: yOrigin (%d) + yExtend (%d) cannot be larger then %d.\n', mfilename, gdal_options.yorigin, gdal_options.yextend, metadata.RasterYSize);
end

% Now, xout and yout are not mandatory. We will set them now, if they are
% not set by the user:
if (gdal_options.xout == metadata.RasterXSize)
	gdal_options.xout = gdal_options.xextend;
end
if (gdal_options.yout == metadata.RasterYSize)
	gdal_options.yout = gdal_options.yextend;
end






