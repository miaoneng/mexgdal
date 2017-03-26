%% 
f = 'data/20030918-093102.img';
m = gdaldump(f);
[x, y, z] = readgdalsimple (f);

%%
z(z < 0) = 0;
z(isnan(z)) = 0;

projection = prjparse(m.ProjectionRef);

%%
X = linspace(x(1), x(2), m.RasterXSize);
Y = linspace(y(1), y(2), m.RasterYSize);
[x1, y1] = meshgrid(X,Y);
[lat,lon] = minvtran(projection, x1, y1);

%%
axesm(projection);
h = pcolorm(lat, lon, z);



