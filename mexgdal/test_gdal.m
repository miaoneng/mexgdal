%% Simple test
f = 'data/20030918-093102.img';
m = gdaldump(f);
[x, y, z] = readgdalsimple (f);
figure;
image(x, y, z);
set(gca, 'YDir', 'Normal')


