function metadata = gdaldump ( gdalfile )
% GDALDUMP:  retrieves metadata from a GDAL raster file
%
% USAGE:  metadata = gdaldump ( gdalfile );
%
% PARAMETERS:
% Input:
%    gdalfile:
%        a raster file that can be read by the GDAL library
% Output:
%    metadata:
%        structure of metadata read from the gdal file.   Fields include
%  
%            ProjectionRef:  
%                this is an empty string if the raster file has no explicit
%                projection associated with it.
%
%            GeoTransform:
%                The affine transform consists of six coefficients which map 
%                pixel/line coordinates into georeferenced space using the 
%                following relationship:
%
%                    Xgeo = GT(1) + Xpixel*GT(2) + Yline*GT(3)
%                    Ygeo = GT(4) + Xpixel*GT(5) + Yline*GT(6)
%
%                In case of north up images, the GT(3) and GT(5) coefficients 
%                are zero, and the GT(2) is pixel width, and GT(6) is pixel 
%                height. The (GT(1),GT(4)) position is the top left corner of 
%                the top left pixel of the raster.
%
%            DriverShortName, DriverLongName:
%                These just tell you what driver will be used to retrieve the
%                data.
%
%            RasterXSize, RasterYSize:
%                Size of the rasters.
%
%            RasterCount:
%                How many raster arrays are in the file.  For example, PNG
%                files could have a red, green, and blue channel, so that 
%                would be three.
%                 
%            Band:
%                A structure of metadata about each raster in the dataset.
%                Fields include
%                
%                  XSize, YSize:
%                      Size of the raster.
%
%                  Overview:
%                      A structure array containing information about each 
%                      overview in this particular band.  Included fields are
%                     
%                         XSize, YSize:
%                             Size of the overview.
%
%                  NoDataValue:
%                      Pixels that are equal to this value would be good 
%                      candidates to change to NaN.
%
%                  DataType:
%                      Should be one of 'Byte', 'UInt16', 'Int16', 
%                      'UInt32', 'Int32', 'Float32', 'Float64', 
%                      
 
options.gdal_dump = 1;
metadata = mexgdal ( gdalfile, options );

return
