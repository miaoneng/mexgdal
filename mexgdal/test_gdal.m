f = 'data/20030918-093102.img';
options.verbose = 1;
m = gdaldump ( f );
[x,y,z] = readgdalsimple ( f );
image(x,y,z);
set ( gca,'YDir', 'normal' );
