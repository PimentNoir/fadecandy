#!/usr/bin/perl -w

use strict;
use OPC;
use Time::HiRes qw/usleep/;

my $num_leds = 64;
my $max_brightness = 100; # Must be between 0 and 255, inclusive

my $client = new OPC('localhost:7890');
$client->can_connect();

###
# This displays a Mandelbrot fractal on the LED strip, one line at a time..
#
# When run with no arguments, it produces the Mandelbrot fractal we all know and love:
#
# ###################@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
# ###############@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
# ############@@@@@@@@@@@@@@@@%%%%%%%%%%%%%%%%%%%%%%%%%@@@@@@@@@@@@@@@@@@@@@@@@@@@
# ##########@@@@@@@@@@%%%%%%%%%%%%%%%%%%%%%%%OOOOO+++OOOO%%%%%%@@@@@@@@@@@@@@@@@@@
# ########@@@@@@@%%%%%%%%%%%%%%%%%%%%%OOOOOOOO+++:=  ~=.+OOOOOO%%%%%@@@@@@@@@@@@@@
# ######@@@@@%%%%%%%%%%%%%%%%%%%%%OOOOOOOOO++++:=~-   .~=:++OOOOOO%%%%%%@@@@@@@@@@
# #####@@@%%%%%%%%%%%%%%%%%%%%OOOOOOOOO+++:::==~,       ,==:+++++OOO%%%%%%%@@@@@@@
# ###@@@%%%%%%%%%%%%%%%%%%%OOOOOO++++:=.  .-,               -~==~ =+OO%%%%%%%@@@@@
# ##@@%%%%%%%%%%%%%%%%OOO+++++++++:::=~-                          ~:+OO%%%%%%%%@@@
# ##@%%%%%%%%%%OOOO+: -:::::::::===~-                             ~~:+OO%%%%%%%%@@
# #@%%%OOOOOOOO++++:=~,         ,--,                               ~:+OOO%%%%%%%%@
# #%OOOOOOOO+++++:=~~,                                             ~:+OOOO%%%%%%%%
# #+++::~==::===--                                               ,=:++OOOO%%%%%%%%
# #+++::~==::===--                                               ,=:++OOOO%%%%%%%%
# #%OOOOOOOO+++++:=~~,                                             ~:+OOOO%%%%%%%%
# #@%%%OOOOOOOO++++:=~,         ,--,                               ~:+OOO%%%%%%%%@
# ##@%%%%%%%%%%OOOO+: -:::::::::===~-                             ~~:+OO%%%%%%%%@@
# ##@@%%%%%%%%%%%%%%%%OOO+++++++++:::=~-                          ~:+OO%%%%%%%%@@@
# ###@@@%%%%%%%%%%%%%%%%%%%OOOOOO++++:=.  .-,               -~==~ =+OO%%%%%%%@@@@@
# #####@@@%%%%%%%%%%%%%%%%%%%%OOOOOOOOO+++:::==~,       ,==:+++++OOO%%%%%%%@@@@@@@
# ######@@@@@%%%%%%%%%%%%%%%%%%%%%OOOOOOOOO++++:=~-   .~=:++OOOOOO%%%%%%@@@@@@@@@@
# ########@@@@@@@%%%%%%%%%%%%%%%%%%%%%OOOOOOOO+++:=  ~=.+OOOOOO%%%%%@@@@@@@@@@@@@@
# ##########@@@@@@@@@@%%%%%%%%%%%%%%%%%%%%%%%OOOOO+++OOOO%%%%%%@@@@@@@@@@@@@@@@@@@
# ############@@@@@@@@@@@@@@@@%%%%%%%%%%%%%%%%%%%%%%%%%@@@@@@@@@@@@@@@@@@@@@@@@@@@
# ###############@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#
# You can provide different real and imaginary centers, and windowing 
# width (in that order) through the command line, though. Such as:
#
#    ./mandelbrot.pl -0.6209 0.6555 0.3727
#
# That one's pretty, too.
###

# Initialize with an empty pixel array
my $pixels = [];
push @$pixels, [0,0,0] while scalar(@$pixels) < $num_leds;
$client->put_pixels(0,$pixels);

# Mandelbrot coefficients
my $width=$num_leds;
my $height=50;
my ($center_real, $center_imaginary, $mandel_width) = @ARGV;
$center_real ||= -0.5;
$center_imaginary ||= 0;
$mandel_width ||= 1.5;

my @pixel_weights = (
	[20,  20,  100],
	[40,  0,   80],
	[60,  0,   60],
	[80,  0,   40],
	[100, 20,  20],
	[80,  40,  0],
	[60,  60,  0],
	[40,  80,  0],
	[20,  100, 20],
	[0,   80,  40],
	[0,   60,  60],
	[0,   40,  80],
);

###
# The main loop
###
while(1){

	my $MaxIter=scalar(@pixel_weights)-1;

	my $MinRe = $center_real - $mandel_width; 
	my $MaxRe = $center_real + $mandel_width;
	my $MinIm = $center_imaginary - $mandel_width; 
	my $MaxIm = $center_imaginary + $mandel_width;

	for (my $Im = $MinIm; $Im <= $MaxIm; $Im += ($MaxIm - $MinIm) / $height) {
		my $pixel_row = [];
		for (my $Re = $MinRe; $Re <= $MaxRe; $Re += ($MaxRe - $MinRe) / $width) {
			my $zr = $Re; 
			my $zi = $Im;
			my $n = 0;
			for ($n = 0; $n < $MaxIter; $n++) {
				my $a = $zr * $zr; $b = $zi * $zi;
				last if $a + $b > 4.0;
				$zi = 2 * $zr * $zi + $Im; 
				$zr = $a - $b + $Re;
			}
			push @$pixel_row, $pixel_weights[$n];
		}
		$client->put_pixels(0,$pixel_row);
		usleep 100000;
	}

	# Now do it backwards! (If we're zoomed in, it'll jitter if we don't)
	for (my $Im = $MaxIm; $Im >= $MinIm; $Im -= ($MaxIm - $MinIm) / $height) {
		my $pixel_row = [];
		for (my $Re = $MinRe; $Re <= $MaxRe; $Re += ($MaxRe - $MinRe) / $width) {
			my $zr = $Re; 
			my $zi = $Im;
			my $n = 0;
			for ($n = 0; $n < $MaxIter; $n++) {
				my $a = $zr * $zr; $b = $zi * $zi;
				last if $a + $b > 4.0;
				$zi = 2 * $zr * $zi + $Im; 
				$zr = $a - $b + $Re;
			}
			push @$pixel_row, $pixel_weights[$n];
		}
		$client->put_pixels(0,$pixel_row);
		usleep 100000;
	}

}

print "Done\n";