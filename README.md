auto-play
=========

repo for auto play feature

PRE-REQUISITE
-------------
1. install ffmpeg and make sure executable can be found in $PATH
2. install cjpeg and make sure executable can be found in $PATH

HOW TO RUN?
----------
./animation -s $size videofile
here $size could be one of the following values:
sqcif -> 128x96
qcif ->	176x144
cif -> 352,288
4cif -> 744x576
16cif -> 1408x1152
qqvga -> 160x120
qvga ->	320x240
vga -> 640x480
svga ->	800x600
xga ->	1024x768
uxga ->	1600x1200
qxga	-> 2048x15336
sxga -> 1280x1024
qsxga -> 2560x2048
hsxga -> 5120x4096
wxga -> 1366x768
wsxga -> 1600x1024
wuxga -> 1920x1200
woxga -> 2560x1600
wqsxga-> 3200,2048
wquxga -> 3840x2400
whsxga -> 6400x4096
whuxga -> 7680x4800
cga ->	320x200

output jpeg files will be in dir names encoder_output
