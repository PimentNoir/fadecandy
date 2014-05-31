#!/usr/bin/perl -w

use strict;
use OPC;

my $num_leds = 64;
my $max_brightness = 100; # Must be between 0 and 255, inclusive

my $client = new OPC('localhost:7890');
$client->can_connect();

###
# This runs a one-dimensional cellular automaton on an LED strip.
#
# Use: ./automaton.pl [rule number].
#
# Rule number chosen must be in 0..255, inclusive.
# If you don't give it a rule number, one will be randomly chosen for you.
###

my $rule = $ARGV[0];
$rule = int(rand(255)) unless $rule;
print "Running rule $rule\n";

###
# &upsample
# Converts an array of brightness levels into corresponding RGB vector
###
sub upsample {
	my ($vector) = @_;
	
	my @vector = map({[$max_brightness * $_, $max_brightness * $_, $max_brightness * $_]} @$vector);
	
	return \@vector
}

# Initialize with a random empty pixel array
my $pixels = [];
push @$pixels, (int(rand(2))) while scalar(@$pixels) < $num_leds;
$client->put_pixels(0,upsample($pixels));

###
# The main loop
###
while(1){
	my $gen = [];
	push @$gen, 0 while scalar(@$gen) < $num_leds;
	foreach my $i (0..$num_leds-1) {
		
		# Orient ourselves
		my $i_left = $i - 1;
		$i_left = $num_leds if $i < 0;
		my $i_right = $i + 1;
		$i_right = 0 if $i_right == $num_leds;
		
		if ($pixels->[$i_left] == 0 && $pixels->[$i] == 0 && $pixels->[$i_right] == 0) {
			$gen->[$i] = 1 if ($rule & (1 << 0));
			
		} elsif ($pixels->[$i_left] == 0 && $pixels->[$i] == 0 && $pixels->[$i_right] > 0) {
			$gen->[$i] = 1 if ($rule & (1 << 1));
			
		} elsif ($pixels->[$i_left] == 0 && $pixels->[$i] > 0 && $pixels->[$i_right] == 0) {
			$gen->[$i] = 1 if ($rule & (1 << 2));

		} elsif ($pixels->[$i_left] == 0 && $pixels->[$i] > 0 && $pixels->[$i_right] > 0) {
			$gen->[$i] = 1 if ($rule & (1 << 3));

		} elsif ($pixels->[$i_left] > 0 && $pixels->[$i] == 0 && $pixels->[$i_right] == 0) {
			$gen->[$i] = 1 if ($rule & (1 << 4));

		} elsif ($pixels->[$i_left] > 0 && $pixels->[$i] == 0 && $pixels->[$i_right] > 0) {
			$gen->[$i] = 1 if ($rule & (1 << 5));

		} elsif ($pixels->[$i_left] > 0 && $pixels->[$i] > 0 && $pixels->[$i_right] == 0) {
			$gen->[$i] = 1 if ($rule & (1 << 6));

		} elsif ($pixels->[$i_left] > 0 && $pixels->[$i] > 0 && $pixels->[$i_right] > 0) {
			$gen->[$i] = 1 if ($rule & (1 << 7));
		}

	}

	$pixels = $gen;
	$client->put_pixels(0,upsample($pixels));
 	sleep 1;
}

print "Done\n";