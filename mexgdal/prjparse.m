function [mstruct] = prjparse(P)
%
% REQUIRED ARG IN: filename
% OPTIONAL ARGS IN: same as shaperead() 
% REQUIRED ARGS OUT: mstruct of projection parameters, S
% OPTIONAL ARG OUT: A
%
% Examples:
%
%  % Read the entire concord_roads.shp shapefile, including the attributes
%  % in concord_roads.dbf. In this example the mstruct will be empty as
%  % there is no projection file.
%  [mstruct S] = shaperead2('concord_roads.shp');
%
% syntax examples only
% [mstruct S] = shaperead2('projectedShapefile.shp');
% [mstruct S A] = shaperead2('geographicShapefile.shp','UseGeoCoords',true);
% Date 2012/11/28

% check for ESRI projection type using first 6 characters of P
if strncmpi(P, 'GEOGCS', 6) || strncmpi(P, 'PROJCS', 6)
    % get projection parameters
    [projN, mProjString, ~] = getProj(P);             
else % neither - eg geocentric
    disp(['Unable to handle projection for ', filename])
end

mstruct = createMstruct(projN, mProjString);

end


function [sProjN, aProjParams, hasProjection] = getProj(P)
% Purpose: split WKT to get projection name, pass to function to look up
% projection parameters. Name is in first set of double quotes.
% Parameters: requires a WKT projection string or a projection name

[~, remain] = strtok(P, '"');   % split WKT prj string at first " delimiter
[pName, ~] = strtok(remain, '"');   % split remain to get just the name

% looking for special cases which need to be fixed as WKT differs
% from matlab esri file. 

if strncmpi(pName, 'WGS_1984', 8) %start of UTM names
    pName = strrep(pName,'WGS_1984','WGS_84');
elseif strncmpi(pName, 'NAD_1983_CSRS', 13) 
    pName = strrep(pName,'NAD_1983_CSRS','NAD83(CSRS98)');
elseif strncmpi(pName, 'GCS_WGS_1984', 12) %geographic
    pName = strrep(pName,'GCS_WGS_1984','WGS_84');
elseif strncmpi(pName, 'NAD_1983_StatePlane', 19) %US StatePlane special
    x = regexpi(pName, 'Feet'); % returns index
    if x > 0 % do nothing, has Feet in name so will match properly
    else % is NAD 83 but no 'Feet' in WKT
        pName = strrep(pName, 'NAD_1983_StatePlane', 'NAD_1983_HARN_StatePlane');
    end
end

disp(['Searching for ',pName,' projection...'])
% projN is the matched name,
% mProjString is a cell array of projection parts
[sProjN, aProjParams] = getProjParams(pName);

if strcmpi(sProjN, '')
    disp([pName, ' projection not found'])
    hasProjection = false;
elseif strcmpi(aProjParams,'')
    disp(['No projection values exist for ', pName])
    hasProjection = false;
else
    disp([sProjN, ' projection found!'])
    hasProjection = true;
end
end

function [projName, projParams] = getProjParams(tName)
% Purpose: Look up the projection from the list of ESRI projections in 
% matlab esri file in map toolbox folder. Get the parameters and create
% an mstruct matching them.
% Parameters: requires a projection name to be passed in. Returns the 
% matching projection name (projName) and a cell array of projection 
% parameters (projString). These are proj4 parameters (eg +proj=tmerc)
%
% ISSUE - the name in the MATLAB esri file does not always match the WKT names 
% in the ESRI projection string. Hence the string manipulation. In particular
% we need to remove forward slashes when comparing. Compares the tline 
% value with the tName using length(tline). This avoids mismatches where 
% the start is the same eg WGS 84 matching to WGS 84 TM 116.

projName = '';
projParams = '';

% MATLAB esri file location
esri_proj = fullfile(matlabroot, 'toolbox', 'map', 'mapproj', 'projdata', 'proj', 'esri');
fidESRI = fopen(esri_proj);
if (fidESRI == -1) % file not found
    error('The Mapping Toolbox ESRI file was not found!')
end
tline = fgetl(fidESRI);

% tidy up input 
% replace underscore (not used in matlab esri projection file)
tName = strrep(tName, '_', ' '); 

while ischar(tline)
    if strncmpi(tline, '# ', 2) % is a projection name
        tline = strrep(tline,'/ ',''); % replace '/ ' as not in WKT
        tline = strrep(tline, '# ', ''); % replace '# ' as not in WKT
        if strncmpi(tline,tName, length(tline)) 
            projName = tline;
            tline2 = fgetl(fidESRI); % get next line
            if tline2(1) == '<'  % if next line starts with a '<' then grab it
                projParams = tline2;
                projParams = regexprep(projParams, '<(\w*)>', ''); % remove <> sections
                projParams = strtrim(projParams);
                % seperate the parts into cell array
                projParams = textscan(projParams,'%s','delimiter',' ');
                
            else % no projection parameters
                projParams = '';
            end
        end
    end
    tline = fgetl(fidESRI);
end
fclose(fidESRI);

end
function [mstruct] = createMstruct(sProjN, aProjParams)
% Purpose: extract the parts from mProjString{1} and build the mstruct

% define default parameters
isOrigin = false;
isEasting = false;
isNorthing = false;
isScaled = false;
isGeoid = false;
isZone = false;

sProj ='pcarree'; % default projection to use
aOrigin = [0 0 0];
nFEasting = 0.0;
nFNorthing = 0.0;
aZone = [];
aGeoid = [1 0]; % default
aMapLimLat = [0 0];
aMapLimLon = [0 0];
nScaleFactor = 1.0;

i = 1;
for i = i:length(aProjParams{1})
    j = aProjParams{1}{i};
    % split at '='
    splitJ = regexp(j,'=','split');
    switch splitJ{1}   % first parameter
        
        case '+proj' % projection
            switch splitJ{2}  % change proj codes to matlab expected
                case 'tmerc'
                    sProj = 'tranmerc';
                case 'merc'
                    sProj = 'mercator';
                case 'lcc'
                    sProj = 'lambertstd';
                case 'laea'
                    sProj = 'eqaazim';
                case 'aea'
                    sProj = 'eqaconicstd';
                case 'aeqd'
                    sProj = 'eqdazim';
                case 'eqdc'
                    sProj = 'eqdconicstd';
                case 'stere'
                    sProj = 'stereo';
                case 'utm'
                    sProj = 'utm';
                case 'cass'
                    sProj = 'cassinistd';
                case 'mill'
                    sProj = 'miller';
                case 'poly'
                    sProj = 'polyconstd';
                case 'robin'
                    sProj = 'robinson';
                case 'sinu'
                    sProj = 'sinusoid';
                case 'longlat' 
                    % has no comparable matlab name, use pcarree
                    sProj = 'pcarree';
                otherwise
                    disp(['No match found for ',splitJ{2},', using default: ',sProj])
            end
            % origin
        case '+lat_0'
            aOrigin(1) = str2double(splitJ{2});
            isOrigin = true;
        case '+lon_0'
            aOrigin(2) = str2double(splitJ{2});
            isOrigin = true;
            
        case '+k'
            nScaleFactor = str2double(splitJ{2});
            isScaled = true;
        case '+x_0'
            nFEasting = str2double(splitJ{2});
            isEasting = true;
        case '+y_0'
            nFNorthing = str2double(splitJ{2});
            isNorthing = true;
            % ellipsoid and adjustment for feet if included
        case '+ellps'
            aGeoid = almanac('earth',splitJ{2});  % get values from ellipsoid name
            aGeoid(1) = 1000*aGeoid(1);
            isGeoid = true;
        case '+to_meter'
            % projection in feet, need to convert semimajor axis as is metre for mstruct
            aGeoid(1) = str2double(splitJ{2})*aGeoid(1);
            
        case '+zone'
            suf = sProjN(end:end);   % zone code does not contain N or S!, use name to find out
            aZone = [splitJ{2},suf];
            [aMapLimLat, aMapLimLon] = utmzone(aZone);  % get limit params for zone
            isZone = true; 
        % not all proj4 parameters appear to be supported by mstruct
        case '+datum'
        case '+pm'
        case '+towgs84'
    end
    
    
end
% add special cases not caught above here
switch sProjN
    case 'World Bonne'
        sProj = 'bonne';
    case 'World Plate Carree' % just in case the default is changed later
        sProj = 'pcarree';
end
% build projection from parameters, all optional so test
disp('creating projection struct')
mstruct = defaultm(sProj);
if isGeoid
    mstruct.geoid = aGeoid;
    disp('geoid set')
end
if isOrigin
    mstruct.origin = aOrigin;
    disp('origin set')
end
if isZone
    mstruct.zone = aZone;
    mstruct.maplatlimit = aMapLimLat;
    mstruct.maplonlimit = aMapLimLon;
    disp('zone set')
end
if isEasting
    mstruct.falseeasting = nFEasting;
    disp('falseeasting set')
end
if isNorthing
    mstruct.falsenorthing = nFNorthing;
    disp('falsenorthing set')
end
if isScaled
    mstruct.scalefactor = nScaleFactor;
    disp('scalefactor set')
end
mstruct = defaultm(mstruct);
end

