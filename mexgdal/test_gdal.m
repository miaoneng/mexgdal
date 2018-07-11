% Simple test
f = 'C:/Users/Administrator/Downloads/grid.global_hqnowrad.20180704T205730Z.NIL.P0D.TILE1@1km.REFDBZ.SFC.grb2';
% m = gdaldump(f);
input_option.grid = 1;
input_option.band = 1;
% [x, y, zz] = readgdalband (f, input_option);
% z = z(30:50, 270:290);
z0 = zz(5500:6000, 4100:4600);
z = z0;
z(z <= 10) = nan;
fprintf('Backgroud\n');
% set(gca, 'YDir', 'Normal');
figure;

b = imagesc(z);
set(b,'AlphaData',~isnan(z));
% colormap('winter');
hold all;

% overlay a contour
fprintf('Contouring\n');
q = z0;
q(q < 0) = 0;
a = contour(q, ...
    'LevelList', (10:5:65), ...
    'Color', 'k', ...
    'ShowText', 'on' ...
);


daspect([1 1 1]);