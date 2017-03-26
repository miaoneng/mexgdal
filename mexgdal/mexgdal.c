
/*================================================================= *
 * MEXGDAL.C
 *     Gateway routine to interface with gdal library.
 *
 * PARAMETERS:
 * Input:
 *    The mex file could be called with
 *
 *    [z, atts] = mexgdal ( gdalfile );
 *
 *    In this case, if there were more than one "band" (such as in
 *    a geotiff), the first band would be read.  In raster data sets
 *    where multiple bands are not appropriate (such as ESRI ascii
 *    grids), this is how it should always be called.
 *
 *    [z, atts] = mexgdal ( gdalfile, j );
 *
 *    This will retrieve the band numbered "j" from the raster file.
 *    The rasters are 1-based, not 0-based, so if there are n bands,
 *    then j can range from 1 to n.  The number of raster bands in
 *    a GDAL raster datafile is returned in the "atts" structure
 *    field "RasterCount".
 *
 *
 *    metadata = mexgdal ( gdalfile, 'gdalinfo' );
 *
 *
 * Output:
 *
 * In case of an error, an exception is thrown.
 *
 *=================================================================*/
/* $Revision: 1.4 $ */
#include "gdal.h"

#include "mex.h"
#include "matrix.h"

int record_geotransform(char* gdal_filename, GDALDatasetH hDataset, double* adfGeoTransform);
int unpack_band(const mxArray* field);
int unpack_overview(const mxArray* field);
int unpack_gdal_dump(const mxArray* field);
void handle_overviews(GDALRasterBandH hBand, mxArray* band_struct);
int unpack_verbose(const mxArray* field);
int unpack_xorigin(const mxArray* field);
int unpack_yorigin(const mxArray* field);
int unpack_xextend(const mxArray* field);
int unpack_yextend(const mxArray* field);
int unpack_xout(const mxArray* field);
int unpack_yout(const mxArray* field);
mxArray* populate_metadata_struct(char*);
int unpack_start_count_stride(const mxArray*, int*);
int unpack_input_options(const mxArray*, int*, int*, int*, int*, int*, int*, int*, int*, int*, int*);

/*
 * If this flag is tripped, then we want to provide debugging output.
 */
int mexgdal_verbose;

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
    /*
     * indices
     * */
    int i, j, index, t_index;
    
    /*
     * Construct error and warning messages using this buffer.
     * */
    char error_msg[500];

    /*
     * Where the origin is defined.  In the gdal API, this is referred to
     * as the offset;
     *
     * They are given the reasonable default values in case we don't
     * specify differently.
     * */
    int xorigin = 0; /* nXOff */
    int yorigin = 0; /* nYOff */

    /*
     * The size of the raster image that is referenced.  For example,
     * if the image is 500x600 and we only wanted the top half,
     *
     *    xextend = 250
     *    yextend = 300
     *
     * In the gdal API, this is called nXSize and nYSize.  They are
     * given initial impossible default values that are changed to
     * reasonable default values when we actually know how big the
     * raster is.  If the user supplied values, then they are used,
     * of course.
     *
     * */
    int xextend = -1;
    int yextend = -1;

    /*
     * The scaled output size.  For example, if the raster image is 500x600
     * and we wish to retrieve all of it, but subset it down by a factor of
     * 10 ...
     *
     *     xout = 50
     *     yout = 60
     *
     * In the gdal API, this is called nBufXSize and nBufYSize.  They are
     * given an initial impossible default value that is changed to
     * reasonable defaults when we actually know how big the raster is.
     * If the user supplied values, then they are used, of course.
     *
     * */
    int xout = -1;
    int yout = -1;

    /*
     * Length of character buffers.
     * */
    int buflen;

    /*
     * name of input file
     * */
    char* gdal_filename;

    /*
     * short cut to the mxArray data
     * */
    double* dptr;

    /*
     * Pointer to matlab raster arrayy
     * */
    mxArray* mxGDALraster;

    /*
     * Pointers to matlab array aliases.
     * */
    mxArray* mx_input_gdal_file;

    int status; /* success or failure */

    /*
     * pointer structure used to query the gdal file.
     * */
    GDALDatasetH hDataset;

    GDALRasterBandH hBand;

    /*
     * GDT Byte?, GDT UInt32?  What is it?
     */
    GDALDataType gdal_type;

    /*
     * Which band do we want to retrieve?
     * */
    int requested_band;

    /*
     * What overview are we to retrieve?  If any at all?
     * */
    int requested_overview;

    /*
     * How big is the output datatype?  We can have either Byte or Float64 for now.
     * This is in bytes.
     */
    int out_type_size = 0;
    unsigned char* transpose_buffer_byte;
    double* transpose_buffer_float64;

    /*
     * The data is to be copied into void_buffer initially.  We then transpose it
     * otherwise it comes out, well, transposed.
     */
    void* void_buffer;
    void* transpose_void_buffer;

    /*
     * size of allocated matlab array.
     */
    mwSize rasterDims[2];

    /*
     * This flag keeps track of whether the default assumptions about the
     * output should be followed.  If no 2nd input argument is given, then
     * this is the case.
     */
    int defaults_are_invoked;

    /*
     * If this flag is tripped, then we only want to return the metadata.
     * */
    int gdal_dump;

    /*
     * The size of the raster.
     * */
    int RasterXSize;
    int RasterYSize;
    int RasterCount;

    /*
     * Default error handle
     */
    CPLErr err;

    /*
     * Set up the defaults.
     */
    defaults_are_invoked = 0; /* Assume the user is going to provide input options. */
    gdal_dump = 0; /* We aren't looking for metadata only. */
    requested_band = 1; /* Get the first band unless we are told otherwise. */
    requested_overview = -1; /* Don't get any overview unless specifically asked for. */
    mexgdal_verbose = 1; /* Don't provide debugging output unless told otherwise. */

    /*
     * Check for proper number of arguments
     */
    if (nlhs != 1) {
        mexErrMsgTxt("Only one output argument is allowed.");
    }
    if (nrhs < 1) {
        mexErrMsgTxt("At least one input argument is required.");
    }
    if (nrhs == 1) {
        defaults_are_invoked = 1;
    }
    if (nrhs > 2) {
        mexErrMsgTxt("No more than two input arguments are allowed.");
    }

    /*
     * The first argument must be character.
     * */
    mx_input_gdal_file = (mxArray*)prhs[0];
    if (mxIsChar(mx_input_gdal_file) != 1) {
        mexErrMsgTxt("Input file name must be a string\n");
    }
    if (mxGetM(mx_input_gdal_file) != 1) {
        mexErrMsgTxt("Input file name must be a row vector, not a column string\n");
    }

    buflen = mxGetN(mx_input_gdal_file) + 1;

    gdal_filename = (char*)mxCalloc(buflen, sizeof(char));

    /*
     * copy the string data from prhs[0] into a C string.
     * */
    status = mxGetString(mx_input_gdal_file, gdal_filename, buflen);
    if (status != 0) {
        mexErrMsgTxt("Not enough space for input file argument.\n");
    }

    /*
     * The 2nd input argument is the structure of options.
     */
    if (nrhs == 2) {

        if (mxIsStruct(prhs[1]) != 1) {
            mexErrMsgTxt("2nd input argument must be a structure.\n");
        }

        unpack_input_options(prhs[1],
            &requested_band,
            &requested_overview,
            &gdal_dump,
            &mexgdal_verbose,
            &xorigin, &yorigin,
            &xextend, &yextend,
            &xout, &yout);
    }

    GDALAllRegister();

    /*
     * If we only want metadata, then don't bother with the raster
     * I/O.
     * */
    if (gdal_dump) {
        plhs[0] = populate_metadata_struct(gdal_filename);
        return;
    }

    /*
     * Open the file.
     * */
    hDataset = GDALOpen(gdal_filename, GA_ReadOnly);
    if (hDataset == NULL) {
        sprintf(error_msg, "Unable to open %s.\n", gdal_filename);
        mexErrMsgTxt(error_msg);
    }

    /*
     * If we requested an overview, get it.
     * */
    hBand = GDALGetRasterBand(hDataset, requested_band);
    if (requested_overview >= 0) {
        hBand = GDALGetOverview(hBand, requested_overview);
    }
    
    /*
     * Get the size of the raster.
     * */
    RasterXSize = GDALGetRasterXSize(hBand);
    RasterYSize = GDALGetRasterYSize(hBand);
    RasterCount = GDALGetRasterCount(hBand);

    /*
     * Check the values for xextend and yextend.  If they are
     * still at the initial impossible default values, reset
     * them to reasonable default values, which would be the
     * size of the band (or overview).
     * */
    if (xextend == -1) {
        /*xextend = GDALGetRasterBandXSize ( hBand );*/
        xextend = RasterXSize;
    }
    if (yextend == -1) {
        yextend = RasterYSize;
    }

    /*
     * Check the values for xout and yout.  If they are still at
     * the initial impossible values, then reset them to reasonable
     * default values, which would be the window size specified
     * by [xy]extend and [xy]origin.
     * */
    if (xout == -1) {
        xout = xextend - xorigin;
    }
    if (yout == -1) {
        yout = yextend - yorigin;
    }

    /*
     * Retrieve the data type so we know how to interpret for matlab.
     *
     * If the data is byte, then treat it as such.  Cast everything else to double.
     */
    gdal_type = GDALGetRasterDataType(hBand);

    /*
     * For debugging purposes, mostly.
     * */
    if (mexgdal_verbose) {

        int bGotMin, bGotMax;
        double adfMinMax[2];

        mexPrintf("data type is %d\n", gdal_type);
        mexPrintf("Block=%dx%d Type=%s, ColorInterp=%s\n",
            xextend, yextend,
            GDALGetDataTypeName(GDALGetRasterDataType(hBand)),
            GDALGetColorInterpretationName(GDALGetRasterColorInterpretation(hBand)));

        adfMinMax[0] = GDALGetRasterMinimum(hBand, &bGotMin);
        adfMinMax[1] = GDALGetRasterMaximum(hBand, &bGotMax);

        /*
         * This seems to just about double the amount of time it takes
         * to run a retrieval...
         * */
        if (!(bGotMin && bGotMax)) {
            GDALComputeRasterMinMax(hBand, TRUE, adfMinMax);
        }

        mexPrintf("Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1]);
        mexPrintf("xOrigin = %d\n", xorigin);
        mexPrintf("yOrigin = %d\n", yorigin);
        mexPrintf("RasterXSize = %d\n", RasterXSize);
        mexPrintf("RasterYSize = %d\n", RasterYSize);
        mexPrintf("xExtend = %d\n", xextend);
        mexPrintf("yExtend = %d\n", yextend);
        mexPrintf("xOut = %d\n", xout);
        mexPrintf("yOut = %d\n", yout);
    }

    if (mexgdal_verbose) {
        mexPrintf("Now reading into buffer...\n");
    }
    switch (gdal_type) {
    case GDT_Byte:
        out_type_size = GDALGetDataTypeSize(GDT_Byte) / 8;
        void_buffer = mxCalloc(xout * yout, out_type_size);
        transpose_void_buffer = mxCalloc(xout * yout, out_type_size);
        transpose_buffer_byte = (unsigned char*)transpose_void_buffer;
        err = GDALRasterIO(hBand, GF_Read,
            xorigin, yorigin,
            xextend, yextend,
            void_buffer,
            xout, yout, GDT_Byte, 0, 0);

        /*
         * Transpose the data.  It comes out bass-ackwards otherwise.
         * */
        for (i = 0; i < yout; ++i) {
            for (j = 0; j < xout; ++j) {
                index = i * xout + j;
                t_index = j * yout + i;
                transpose_buffer_byte[t_index] = ((unsigned char*)void_buffer)[index];
            }
        }

        rasterDims[0] = yout;
        rasterDims[1] = xout;
        mxGDALraster = mxCreateNumericArray(2, rasterDims, mxUINT8_CLASS, mxREAL);
        break;

    case GDT_UInt16:
    case GDT_Int16:
    case GDT_UInt32:
    case GDT_Int32:
    case GDT_Float32:
    case GDT_Float64:

        out_type_size = GDALGetDataTypeSize(GDT_Float64) / 8;
        void_buffer = mxCalloc(xout * yout, out_type_size);
        transpose_void_buffer = mxCalloc(xout * yout, out_type_size);
        transpose_buffer_float64 = (double*)transpose_void_buffer;
        err = GDALRasterIO(hBand, GF_Read, xorigin, yorigin,
            xextend, yextend, void_buffer,
            xout, yout, GDT_Float64, 0, 0);

        /*
         * Transpose the data.  It comes out bass-ackwards otherwise.
         * */
        for (i = 0; i < yout; ++i) {
            for (j = 0; j < xout; ++j) {
                index = i * xout + j;
                t_index = j * yout + i;
                transpose_buffer_float64[t_index] = ((double*)void_buffer)[index];
            }
        }

        rasterDims[0] = yout;
        rasterDims[1] = xout;
        mxGDALraster = mxCreateNumericArray(2, rasterDims, mxDOUBLE_CLASS, mxREAL);

        break;
    default:
        sprintf(error_msg, "Unhandled GDALDataType %d.\n", gdal_type);
        mexErrMsgTxt(error_msg);
        return;
    }

    /*
     * Now copy from the transposed array.
     */
    if (mexgdal_verbose) {
        mexPrintf("Now copying into matlab array...\n");
    }

    dptr = mxGetPr(mxGDALraster);
    memcpy((void*)dptr, transpose_void_buffer, out_type_size * xout * yout);

    if (mexgdal_verbose) {
        mexPrintf("Finished copying into matlab array...\n");
    }

    plhs[0] = mxGDALraster;

    GDALClose(hDataset);
    return;
}

/*
 * record_geotransform:
 *
 * If the gdal file is not internally georeferenced, try to get the world file.
 * Returns -1 in case no world file is found.
 * */
int record_geotransform(char* gdal_filename, GDALDatasetH hDataset, double* adfGeoTransform)
{

    int status = -1;
    char generic_buffer[5000];

    if (GDALGetGeoTransform(hDataset, adfGeoTransform) == CE_None) {
        /*
           mexPrintf( "Origin = (%.6f,%.6f)\n",
           adfGeoTransform[0], adfGeoTransform[3] );
           mexPrintf( "Pixel Size = (%.6f,%.6f)\n", adfGeoTransform[1], adfGeoTransform[5] );
           */
        return (0);
    }

    /*
     * Try a world file.  First the generic extension.
     * If the gdal_filename is, say, "a.tif", then this
     * will look for "a.wld".
     * */
    if (GDALReadWorldFile(gdal_filename, "wld", adfGeoTransform)) {
        return (0);
    }

    /*
     * Try again, but try "a.tif.wld" instead.
     * */
    sprintf(generic_buffer, "%s.xxx", gdal_filename);
    status = GDALReadWorldFile(generic_buffer, "wld", adfGeoTransform);
    if (status == 1) {
        return (0);
    }

    /*
     * Newer versions of GDAL will try to guess if you pass NULL.  Older
     * versions with barf, so becareful about attempting this.
     * */
    if (GDAL_VERSION_NUM >= 1210) {
        if (GDALReadWorldFile(gdal_filename, NULL, adfGeoTransform)) {
            return (0);
        }
    }
    return (-1);
}

/*
 * UNPACK_GDAL_DUMP - check the gdal_dump specification.  It's a string, we need
 * to determine if it is "0" or not.
 */
int unpack_gdal_dump(const mxArray* field)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* dptr; /* shortcut to the mxArray's data */

    if (mxIsDouble(field) != 1) {
        sprintf(err_buffer, "unpack_gdal_dump:  gdal_dump field must be a double.\n");
        mexWarnMsgTxt(err_buffer);
    }
    m = mxGetM(field);
    n = mxGetN(field);
    if (m != 1) {
        sprintf(err_buffer, "unpack_gdal_dump:  gdal_dump field must be mx1 rather than %dx%d.\n", m, n);
        mexWarnMsgTxt(err_buffer);
    }

    dptr = mxGetPr(field);
    return ((int)dptr[0]);
}

/*
 * UNPACK_BAND - check the band parameter for consistency and return it.
 */
int unpack_band(const mxArray* field)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* pr;

    m = mxGetM(field);
    n = mxGetN(field);
    if ((m != 1) || (n != 1)) {
        sprintf(err_buffer, "unpack_band:  band field must be 1x1 rather than %dx%d.\n", m, n);
        mexWarnMsgTxt(err_buffer);
    }

    pr = mxGetPr(field);
    return ((int)pr[0]);
}

/*
 * UNPACK_XEXTEND - check the xExtend parameter for consistency and return it.
 */
int unpack_xextend(const mxArray* field)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* pr;

    m = mxGetM(field);
    n = mxGetN(field);
    if ((m != 1) || (n != 1)) {
        sprintf(err_buffer, "unpack_xExtend:  xExtend field must be 1x1 rather than %dx%d.\n", m, n);
        mexErrMsgTxt(err_buffer);
    }

    pr = mxGetPr(field);
    return ((int)pr[0]);
}

/*
 * UNPACK_YEXTEND - check the yExtend parameter for consistency and return it.
 */
int unpack_yextend(const mxArray* field)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* pr;

    m = mxGetM(field);
    n = mxGetN(field);
    if ((m != 1) || (n != 1)) {
        sprintf(err_buffer, "unpack_yExtend:  yExtend field must be 1x1 rather than %dx%d.\n", m, n);
        mexErrMsgTxt(err_buffer);
    }

    pr = mxGetPr(field);
    return ((int)pr[0]);
}

/*
 * UNPACK_XOUT - check the xout parameter for consistency and return it.
 */
int unpack_xout(const mxArray* field)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* pr;

    m = mxGetM(field);
    n = mxGetN(field);
    if ((m != 1) || (n != 1)) {
        sprintf(err_buffer, "unpack_xout:  xout field must be 1x1 rather than %dx%d.\n", m, n);
        mexErrMsgTxt(err_buffer);
    }

    pr = mxGetPr(field);
    return ((int)pr[0]);
}

/*
 * UNPACK_YOUT - check the yout parameter for consistency and return it.
 */
int unpack_yout(const mxArray* field)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* pr;

    m = mxGetM(field);
    n = mxGetN(field);
    if ((m != 1) || (n != 1)) {
        sprintf(err_buffer, "unpack_yout:  yout field must be 1x1 rather than %dx%d.\n", m, n);
        mexErrMsgTxt(err_buffer);
    }

    pr = mxGetPr(field);
    return ((int)pr[0]);
}

/*
 * UNPACK_OVERVIEW - check the overview parameter for consistency and return it.
 */
int unpack_overview(const mxArray* field)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* pr;

    m = mxGetM(field);
    n = mxGetN(field);
    if (m != 1) {
        sprintf(err_buffer, "unpack_overview:  overview field must be 1x1 rather than %dx%d.\n", m, n);
        mexWarnMsgTxt(err_buffer);
    }

    pr = mxGetPr(field);
    return ((int)pr[0]);
}

/*
 * UNPACK_VERBOSE - check the verbose field for consistency and return it.
 *
 * PARAMETERS:
 * Input:
 *     field:
 *         mxArray whose name is "verbose".  It should have a numberic
 *         value, either zero or not zero.
 * Output:
 *     The function returns an integer value, which can be interpreted
 *     as true or false.
 */
int unpack_verbose(const mxArray* field)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* pr;

    m = mxGetM(field);
    n = mxGetN(field);
    if (m != 1) {
        sprintf(err_buffer, "unpack_verbose:  overview field must be 1x1 rather than %dx%d.\n", m, n);
        mexWarnMsgTxt(err_buffer);
    }

    pr = mxGetPr(field);
    return ((int)pr[0]);
}

/*
 * POPULATE_METADATA_STRUCT
 *
 * This routine just queries the GDAL raster file for all the metadata
 * that can be squeezed out of it.
 *
 * The resulting matlab structure is by necessity nested.  Each raster
 * file can have several bands, e.g. PNG files usually have 3, a red, a
 * blue, and a green channel.  Each band can have several overviews (tiffs
 * come to mind here).
 *
 * Fields:
 *    ProjectionRef:  a string describing the projection.  Not parsed.
 *    GeoTransform:
 *        a 6-tuple.  Entries are as follows.
 *            [0] --> top left x
 *            [1] --> w-e pixel resolution
 *            [2] --> rotation, 0 if image is "north up"
 *            [3] --> top left y
 *            [4] --> rotation, 0 if image is "north up"
 *            [5] --> n-s pixel resolution
 *
 *    DriverShortName:  describes the driver used to query *this* raster file
 *    DriverLongName:  describes the driver used to query *this* raster file
 *    RasterXSize, RasterYSize:
 *        These are the primary dimensions of the raster.  See "Overview", though.
 *    RasterCount:
 *        Number of raster bands present in the file.
 *    Driver:
 *        This itself is a structure array.  Each element describes a driver
 *        that the locally compiled GDAL library has available.  So you recompile
 *        GDAL with new format support, this structure will change.
 *
 *        Fields:
 *            DriverShortName, DriverLongName:
 *                Same as fields in top level structure with same name.
 *
 *    Band:
 *        Also a structure array.  One element for each raster band present in
 *        the GDAL file.  See "RasterCount".
 *
 *        Fields:
 *            XSize, YSize:
 *                Dimensions of the current raster band.
 *            Overview:
 *                A structure array, one element for each overview present.  If
 *                empty, then there are no overviews.
 *            NoDataValue:
 *                When passed back to MATLAB, one can set pixels with this value
 *                to NaN.
 *
 * */
mxArray* populate_metadata_struct(char* gdal_filename)
{
    /*
     * Number of available drivers for the version of GDAL we are using.
     * */
    int driverCount;

    /*
     * These are used to define the metadata structure about available GDAL drivers.
     * */
    char* driver_fieldnames[100];
    int num_driver_fields;

    mxArray* driver_struct;
    mxArray* mxtmp;
    mxArray* mxProjectionRef;
    mxArray* mxGeoTransform;
    mxArray* mxGDALDriverShortName;
    mxArray* mxGDALDriverLongName;
    mxArray* mxGDALRasterCount;
    mxArray* mxGDALRasterXSize;
    mxArray* mxGDALRasterYSize;

    /*
     * These will be matlab structures that hold the metadata.
     * "metadata_struct" actually encompasses "band_struct",
     * which encompasses "overview_struct"
     * */
    mxArray* metadata_struct;
    mxArray* band_struct;

    /*
     * Loop indices
     * */
    int j, band_number;

    /*
     * short cut to the mxArray data
     * */
    double* dptr;

    /*
     * This is the driver chosen by the GDAL library
     * to query the dataset.
     * */
    GDALDriverH hDriver;

    /*
     * pointer structure used to query the gdal file.
     * */
    GDALDatasetH hDataset;
    GDALRasterBandH hBand;

    /*
     * Construct error and warning messages using this buffer.
     * */
    char error_msg[500];

    int status; /* success or failure */

    /*
     * bounds on the dataset
     * */
    double adfGeoTransform[6];

    /*
     * number of fields in the metadata structures.
     * */
    int num_struct_fields;
    int num_band_fields;

    /*
     * this array contains the names of the fields of the metadata structure.
     * */
    char* fieldnames[100];
    char* band_fieldnames[100];

    /*
     * Dimensions of the dataset
     * */
    int xSize, ySize, raster_count;

    /*
     * Datatype of the bands.
     * */
    GDALDataType gdal_type;

    /* temporary value */
    double tmpdble;

    /*
     * Retrieve information on all drivers.
     * */
    driverCount = GDALGetDriverCount();

    /*
     * Open the file.
     * */
    hDataset = GDALOpen(gdal_filename, GA_ReadOnly);
    if (hDataset == NULL) {
        sprintf(error_msg, "Unable to open %s.\n", gdal_filename);
        mexErrMsgTxt(error_msg);
    }

    /*
     * Create the metadata structure
     * Just one element, with XXX fields.
     * */
    num_struct_fields = 9;
    fieldnames[0] = strdup("ProjectionRef");
    fieldnames[1] = strdup("GeoTransform");
    fieldnames[2] = strdup("DriverShortName");
    fieldnames[3] = strdup("DriverLongName");
    fieldnames[4] = strdup("RasterXSize");
    fieldnames[5] = strdup("RasterYSize");
    fieldnames[6] = strdup("RasterCount");
    fieldnames[7] = strdup("Driver");
    fieldnames[8] = strdup("Band");
    metadata_struct = mxCreateStructMatrix(1, 1, num_struct_fields, (const char**)fieldnames);

    num_driver_fields = 2;
    driver_fieldnames[0] = strdup("DriverLongName");
    driver_fieldnames[1] = strdup("DriverShortName");
    driver_struct = mxCreateStructMatrix(driverCount, 1, num_driver_fields, (const char**)driver_fieldnames);
    for (j = 0; j < driverCount; ++j) {
        hDriver = GDALGetDriver(j);
        mxtmp = mxCreateString(GDALGetDriverLongName(hDriver));
        mxSetField(driver_struct, j, (const char*)"DriverLongName", mxtmp);

        mxtmp = mxCreateString(GDALGetDriverShortName(hDriver));
        mxSetField(driver_struct, j, (const char*)"DriverShortName", mxtmp);
    }
    mxSetField(metadata_struct, 0, "Driver", driver_struct);

    /*
     * Record the ProjectionRef.
     * */
    mxProjectionRef = mxCreateString(GDALGetProjectionRef(hDataset));
    mxSetField(metadata_struct, 0, "ProjectionRef", mxProjectionRef);

    /*
     * Record the geotransform.
     * */
    mxGeoTransform = mxCreateNumericMatrix(6, 1, mxDOUBLE_CLASS, mxREAL);
    dptr = mxGetPr(mxGeoTransform);

    status = record_geotransform(gdal_filename, hDataset, adfGeoTransform);
    if (status == 0) {
        dptr[0] = adfGeoTransform[0];
        dptr[1] = adfGeoTransform[1];
        dptr[2] = adfGeoTransform[2];
        dptr[3] = adfGeoTransform[3];
        dptr[4] = adfGeoTransform[4];
        dptr[5] = adfGeoTransform[5];
        mxSetField(metadata_struct, 0, "GeoTransform", mxGeoTransform);
    }
    else {
        sprintf(error_msg, "No internal georeferencing exists for %s, and could not find a suitable world file either.\n", gdal_filename);
        mexWarnMsgTxt(error_msg);
    }

    /*
     * Get driver information
     * */
    hDriver = GDALGetDatasetDriver(hDataset);

    /*
       mexPrintf( "Driver: %s/%s\n",
       GDALGetDriverShortName( hDriver ),
       GDALGetDriverLongName( hDriver ) );
       */
    mxGDALDriverShortName = mxCreateString(GDALGetDriverShortName(hDriver));
    mxSetField(metadata_struct, 0, (const char*)"DriverShortName", mxGDALDriverShortName);

    mxGDALDriverLongName = mxCreateString(GDALGetDriverLongName(hDriver));
    mxSetField(metadata_struct, 0, (const char*)"DriverLongName", mxGDALDriverLongName);

    xSize = GDALGetRasterXSize(hDataset);
    mxGDALRasterXSize = mxCreateDoubleScalar((double)xSize);
    mxSetField(metadata_struct, 0, (const char*)"RasterXSize", mxGDALRasterXSize);

    ySize = GDALGetRasterYSize(hDataset);
    mxGDALRasterYSize = mxCreateDoubleScalar((double)ySize);
    mxSetField(metadata_struct, 0, (const char*)"RasterYSize", mxGDALRasterYSize);

    raster_count = GDALGetRasterCount(hDataset);
    mxGDALRasterCount = mxCreateDoubleScalar((double)raster_count);
    mxSetField(metadata_struct, 0, (const char*)"RasterCount", mxGDALRasterCount);
    /*
       mexPrintf( "Size is %dx%dx%d\n", xSize, ySize, raster_count );
       */

    /*
     * Get the metadata for each band.
     * */
    num_band_fields = 5;
    band_fieldnames[0] = strdup("XSize");
    band_fieldnames[1] = strdup("YSize");
    band_fieldnames[2] = strdup("Overview");
    band_fieldnames[3] = strdup("NoDataValue");
    band_fieldnames[4] = strdup("DataType");
    band_struct = mxCreateStructMatrix(raster_count, 1, num_band_fields, (const char**)band_fieldnames);

    for (band_number = 1; band_number <= raster_count; ++band_number) {

        hBand = GDALGetRasterBand(hDataset, band_number);

        mxtmp = mxCreateDoubleScalar((double)GDALGetRasterBandXSize(hBand));
        mxSetField(band_struct, 0, "XSize", mxtmp);

        mxtmp = mxCreateDoubleScalar((double)GDALGetRasterBandYSize(hBand));
        mxSetField(band_struct, 0, "YSize", mxtmp);

        gdal_type = GDALGetRasterDataType(hBand);

        mxtmp = mxCreateString(GDALGetDataTypeName(gdal_type));
        mxSetField(band_struct, 0, (const char*)"DataType", mxtmp);

        tmpdble = GDALGetRasterNoDataValue(hBand, &status);
        mxtmp = mxCreateDoubleScalar((double)(GDALGetRasterNoDataValue(hBand, &status)));
        mxSetField(band_struct, 0, "NoDataValue", mxtmp);

        /*
         * Can have multiple overviews per band.
         * */
        handle_overviews(hBand, band_struct);
    }

    mxSetField(metadata_struct, 0, "Band", band_struct);

    GDALClose(hDataset);

    return (metadata_struct);
}

/*
 * HANDLE_OVERVIEWS
 *
 * If the raster file has overviews, then we need to populate the
 * metadata structure appropriately.
 * */
void handle_overviews(GDALRasterBandH hBand, mxArray* band_struct)
{

    /*
     * Number of metadata items for each overview structure.
     * */
    int num_overview_fields = 2;

    char* overview_fieldnames[2];

    /*
     * Just a temporary matlab array structure.  We don't keep it around.
     * It will hold the X and Y sizes of the overviews.
     * */
    mxArray* mxtmp;

    /*
     * This matlab structure will contain the size of the overview
     * in fields "XSize" and "YSize"
     * */
    mxArray* overview_struct;

    /*
     * GDAL datastructure for the overview.
     * */
    GDALRasterBandH overview_hBand;

    /*
     * Dimensions of the overview
     * */
    int xSize, ySize;

    /*
     * Number of overviews in the current band.
     * */
    int num_overviews;

    /*
     * loop index
     * */
    int overview;

    /*
     * These are the only fields defined for the overview metadata.
     * */
    overview_fieldnames[0] = strdup("XSize");
    overview_fieldnames[1] = strdup("YSize");

    num_overviews = GDALGetOverviewCount(hBand);
    if (num_overviews > 0) {

        overview_struct = mxCreateStructMatrix(num_overviews, 1, num_overview_fields,
            (const char**)overview_fieldnames);

        for (overview = 0; overview < num_overviews; ++overview) {
            overview_hBand = GDALGetOverview(hBand, overview);

            xSize = GDALGetRasterBandXSize(overview_hBand);
            mxtmp = mxCreateDoubleScalar(xSize);
            mxSetField(overview_struct, overview, "XSize", mxtmp);

            ySize = GDALGetRasterBandYSize(overview_hBand);
            mxtmp = mxCreateDoubleScalar(ySize);
            mxSetField(overview_struct, overview, "YSize", mxtmp);
        }
        mxSetField(band_struct, 0, "Overview", overview_struct);
    }
}

/*
 * Unpack a 2-tuple.
 * */
int unpack_start_count_stride(const mxArray* field, int* start_count_stride)
{

    char err_buffer[500]; /* debugging and error reporting purposes */
    int m, n; /* size of insys parameter */
    double* pr;

    m = mxGetM(field);
    n = mxGetN(field);
    if (m != 1) {
        sprintf(err_buffer, "unpack_start_count_stride:  start field must be 1x2 rather than %dx%d.\n", m, n);
        mexWarnMsgTxt(err_buffer);
        return (-1);
    }
    if (n != 2) {
        sprintf(err_buffer, "unpack_start_count_stride:  start field must be 1x2 rather than %dx%d.\n", m, n);
        mexWarnMsgTxt(err_buffer);
        return (-1);
    }

    pr = mxGetPr(field);
    start_count_stride[0] = (int)pr[0];
    start_count_stride[1] = (int)pr[1];
    return (0);
}

/*
 * UNPACK_INPUT_OPTIONS
 *
 * Unpack all the fields from the input structure.
 * */
int unpack_input_options(const mxArray* mx_struct,
    int* requested_band,
    int* requested_overview,
    int* gdal_dump,
    int* verbose,
    int* xorigin, int* yorigin,
    int* xextend, int* yextend,
    int* xout, int* yout)
{

    /*
     * Number of fields in the input options structure.
     * */
    int nfields;

    /*
     * Loop index for going thru the fields.
     * */
    int ifield;

    /*
     * Name of the field pointed to by ifield.
     * */
    char* fieldname;

    /*
     * Pointers to members of the attribute structure.
     * */
    mxArray* mxField;

    /*
     * Construct error and warning messages using this buffer.
     * */
    char error_msg[500];
    int status = 0;

    /*
     * Go thru each of the structures and retrieve the parameters.
     * */
    nfields = mxGetNumberOfFields(mx_struct);
    for (ifield = 0; ifield < nfields; ++ifield) {
        mxField = mxGetFieldByNumber(mx_struct, 0, ifield);
        if (mxField == NULL) {
            sprintf(error_msg, "mxGetFieldByNumber returned NULL on field %d.\n", ifield);
            mexErrMsgTxt(error_msg);
        }

        fieldname = (char*)mxGetFieldNameByNumber(mx_struct, ifield);
        if (fieldname == NULL) {
            sprintf(error_msg, "mxGetFieldNameByNumber returned NULL on field %d.\n", ifield);
            mexErrMsgTxt(error_msg);
        }

        if (strcmp(fieldname, "band") == 0) {
            *requested_band = unpack_band(mxField);
        }

        if (strcmp(fieldname, "overview") == 0) {
            *requested_overview = unpack_overview(mxField);
        }

        if (strcmp(fieldname, "gdal_dump") == 0) {
            *gdal_dump = unpack_gdal_dump(mxField);
        }

        if (strcmp(fieldname, "verbose") == 0) {
            *verbose = unpack_verbose(mxField);
        }

        if (strcmp(fieldname, "xorigin") == 0) {
            *xorigin = unpack_band(mxField);
        }

        if (strcmp(fieldname, "yorigin") == 0) {
            *yorigin = unpack_band(mxField);
        }

        if (strcmp(fieldname, "xextend") == 0) {
            *xextend = unpack_xextend(mxField);
        }

        if (strcmp(fieldname, "yextend") == 0) {
            *yextend = unpack_yextend(mxField);
        }

        if (strcmp(fieldname, "xout") == 0) {
            *xout = unpack_xout(mxField);
        }

        if (strcmp(fieldname, "yout") == 0) {
            *yout = unpack_yout(mxField);
        }
    }
    return (status);
}