#!/usr/bin/perl -w

use strict;
use OPC;

my $num_leds = 64;
my $max_brightness = 100; # Must be between 0 and 255, inclusive

my $client = new OPC('localhost:7890');
$client->can_connect();

###
# This is an imlpementation of a 2d game of life for a 1d LED strip.
#
# The first dimension is the position of the LED: zero to $num_leds.
# The second dimension is the color of the LED: zero to nine.
#    * these display as nine quantized hues along an RGB axis..
#    * For the purposes of completeness, red and violet act as neighbors.
#
# Just run it. It's beautiful.
###

my $pixels = [];

# Initialize with a random empty pixel array
push @$pixels, [int(rand(2)),int(rand(2)),int(rand(2)),
                int(rand(2)),int(rand(2)),int(rand(2)),
                int(rand(2)),int(rand(2)),int(rand(2))] while scalar(@$pixels) < $num_leds;

$client->put_pixels(0,downsample($pixels));


###
# &downsample
# Takes a seven-element array of colors-points
# Returns "equivalent" array of R,B,G colors
###
sub downsample {
	my ($pixels) = @_;
	my $output = [];
	
	foreach my $i (0..scalar(@$pixels)-1) {
		my $vector = [0,0,0];
		
		$vector->[0] = ( $pixels->[$i]->[8] / 3) +
		           ( 2 * $pixels->[$i]->[0] / 3) + 
		                 $pixels->[$i]->[1] + 
		           ( 2 * $pixels->[$i]->[2] / 3) +
		               ( $pixels->[$i]->[3] / 3);

		$vector->[1] = ( $pixels->[$i]->[2] / 3) +
		           ( 2 * $pixels->[$i]->[3] / 3) + 
		                 $pixels->[$i]->[4] + 
		           ( 2 * $pixels->[$i]->[5] / 3) +
		               ( $pixels->[$i]->[6] / 3);

		$vector->[2] = ( $pixels->[$i]->[5] / 3) +
		           ( 2 * $pixels->[$i]->[6] / 3) + 
		                 $pixels->[$i]->[7] + 
		           ( 2 * $pixels->[$i]->[8] / 3) +
		               ( $pixels->[$i]->[0] / 3);
		
		foreach my $j (0..2){
			$vector->[$j] = int($max_brightness * ($vector->[$j] / 2));
		}
		
		push @$output, $vector;
	}
	return $output;
}

###
# The main loop
###
while(1){
	my $gen = [];
	push @$gen, [0,0,0,0,0,0,0,0,0] while scalar(@$gen) < $num_leds;

	foreach my $i (0..$num_leds-1) {
		foreach my $j (0..8){
			
			### Orient ourselves
			my $i_up = $i + 1;
			$i_up = 0 if $i_up == $num_leds;
			my $i_down = $i - 1;
			$i_down = $num_leds - 1 if $i_down < 0;
			my $j_right = $j + 1;
			$j_right = 0 if $j_right == 9;
			my $j_left = $j - 1;
			$j_left = 8 if $j_left < 0;
			
			### How many neighbors do we have?
			my $neighbors = $pixels->[$i_up]->[$j_right] +
							$pixels->[$i_up]->[$j] +
							$pixels->[$i_up]->[$j_left] +
							$pixels->[$i]->[$j_left] +
							$pixels->[$i_down]->[$j_left] +
							$pixels->[$i_down]->[$j] +
							$pixels->[$i_down]->[$j_right] +
							$pixels->[$i]->[$j_right];
			
			# Any live cell with fewer than two live neighbours dies, as if caused by under-population.
			if ($pixels->[$i] && $neighbors < 2) {
				$gen->[$i]->[$j] = 0;

			# Any live cell with two or three live neighbours lives on to the next generation.
			} elsif ($pixels->[$i] && ($neighbors == 2 || $neighbors == 3)) {
				$gen->[$i]->[$j] = 1;

			# Any live cell with more than three live neighbours dies, as if by overcrowding.
			} elsif ($pixels->[$i] && $neighbors > 3) {
				$gen->[$i]->[$j] = 0;
				
			# Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
			} elsif (!$pixels->[$i] && $neighbors == 3) {
				$gen->[$i]->[$j] = 1;
			}
		}
	}

	$pixels = $gen;
	$client->put_pixels(0,downsample($pixels));
 	sleep 1;
}

print "Done\n";