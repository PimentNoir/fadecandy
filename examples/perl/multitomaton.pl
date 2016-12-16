#!/usr/bin/perl -w

use strict;
use OPC;

my $num_leds = 64;
my $max_brightness = 100; # Must be between 0 and 255, inclusive

my $client = new OPC('localhost:7890');
$client->can_connect();

###
# This runs three one-dimensional cellular automata on an LED strip.
# One runs on red, one on blue, the other on green.
#
# Use: ./automaton.pl [red rule number] [green rule number] [blue rule number].
#
# Rule numbers chosen must be in 0..255, inclusive.
# If you don't give it a rule number, numbers will be randomly chosen for you.
###

my $red_rule = $ARGV[0];
$red_rule = int(rand(255)) unless $red_rule;

my $green_rule = $ARGV[1];
$green_rule = int(rand(255)) unless $green_rule;

my $blue_rule = $ARGV[2];
$blue_rule = int(rand(255)) unless $blue_rule;

print "Running rules $red_rule (red), $green_rule (green), and $blue_rule (blue) \n";

# Initialize with a random empty pixel array
my $pixels = [];
push @$pixels, ([$max_brightness * int(rand(2)),
				 $max_brightness * int(rand(2)),
				 $max_brightness * int(rand(2))]) 
			   while scalar(@$pixels) < $num_leds;

$client->put_pixels(0,$pixels);

###
# The main loop
###
while(1){
	my $gen = [];
	push @$gen, [0,0,0] while scalar(@$gen) < $num_leds;
	foreach my $i (0..$num_leds-1) {
		
		# Orient ourselves
		my $i_left = $i - 1;
		$i_left = $num_leds if $i < 0;
		my $i_right = $i + 1;
		$i_right = 0 if $i_right == $num_leds;

		# Calculate the red automaton
		if ($pixels->[$i_left]->[0] == 0 && $pixels->[$i]->[0] == 0 && $pixels->[$i_right]->[0] == 0) {
			$gen->[$i]->[0] = $max_brightness if ($red_rule & (1 << 0));
		} elsif ($pixels->[$i_left]->[0] == 0 && $pixels->[$i]->[0] == 0 && $pixels->[$i_right]->[0] > 0) {
			$gen->[$i]->[0] = $max_brightness if ($red_rule & (1 << 1));			
		} elsif ($pixels->[$i_left]->[0] == 0 && $pixels->[$i]->[0] > 0 && $pixels->[$i_right]->[0] == 0) {
			$gen->[$i]->[0] = $max_brightness if ($red_rule & (1 << 2));
		} elsif ($pixels->[$i_left]->[0] == 0 && $pixels->[$i]->[0] > 0 && $pixels->[$i_right]->[0] > 0) {
			$gen->[$i]->[0] = $max_brightness if ($red_rule & (1 << 3));
		} elsif ($pixels->[$i_left]->[0] > 0 && $pixels->[$i]->[0] == 0 && $pixels->[$i_right]->[0] == 0) {
			$gen->[$i]->[0] = $max_brightness if ($red_rule & (1 << 4));
		} elsif ($pixels->[$i_left]->[0] > 0 && $pixels->[$i]->[0] == 0 && $pixels->[$i_right]->[0] > 0) {
			$gen->[$i]->[0] = $max_brightness if ($red_rule & (1 << 5));
		} elsif ($pixels->[$i_left]->[0] > 0 && $pixels->[$i]->[0] > 0 && $pixels->[$i_right]->[0] == 0) {
			$gen->[$i]->[0] = $max_brightness if ($red_rule & (1 << 6));
		} elsif ($pixels->[$i_left]->[0] > 0 && $pixels->[$i]->[0] > 0 && $pixels->[$i_right]->[0] > 0) {
			$gen->[$i]->[0] = $max_brightness if ($red_rule & (1 << 7));
		}

		# Calculate the green automaton
		if ($pixels->[$i_left]->[1] == 0 && $pixels->[$i]->[1] == 0 && $pixels->[$i_right]->[1] == 0) {
			$gen->[$i]->[1] = $max_brightness if ($green_rule & (1 << 0));
		} elsif ($pixels->[$i_left]->[1] == 0 && $pixels->[$i]->[1] == 0 && $pixels->[$i_right]->[1] > 0) {
			$gen->[$i]->[1] = $max_brightness if ($green_rule & (1 << 1));
		} elsif ($pixels->[$i_left]->[1] == 0 && $pixels->[$i]->[1] > 0 && $pixels->[$i_right]->[1] == 0) {
			$gen->[$i]->[1] = $max_brightness if ($green_rule & (1 << 2));
		} elsif ($pixels->[$i_left]->[1] == 0 && $pixels->[$i]->[1] > 0 && $pixels->[$i_right]->[1] > 0) {
			$gen->[$i]->[1] = $max_brightness if ($green_rule & (1 << 3));
		} elsif ($pixels->[$i_left]->[1] > 0 && $pixels->[$i]->[1] == 0 && $pixels->[$i_right]->[1] == 0) {
			$gen->[$i]->[1] = $max_brightness if ($green_rule & (1 << 4));
		} elsif ($pixels->[$i_left]->[1] > 0 && $pixels->[$i]->[1] == 0 && $pixels->[$i_right]->[1] > 0) {
			$gen->[$i]->[1] = $max_brightness if ($green_rule & (1 << 5));
		} elsif ($pixels->[$i_left]->[1] > 0 && $pixels->[$i]->[1] > 0 && $pixels->[$i_right]->[1] == 0) {
			$gen->[$i]->[1] = $max_brightness if ($green_rule & (1 << 6));
		} elsif ($pixels->[$i_left]->[1] > 0 && $pixels->[$i]->[1] > 0 && $pixels->[$i_right]->[1] > 0) {
			$gen->[$i]->[1] = $max_brightness if ($green_rule & (1 << 7));
		}
		
		# Calculate the blue automaton
		if ($pixels->[$i_left] == 0 && $pixels->[$i] == 0 && $pixels->[$i_right] == 0) {
			$gen->[$i]->[2] = $max_brightness if ($blue_rule & (1 << 0));
		} elsif ($pixels->[$i_left]->[2] == 0 && $pixels->[$i]->[2] == 0 && $pixels->[$i_right]->[2] > 0) {
			$gen->[$i]->[2] = $max_brightness if ($blue_rule & (1 << 1));			
		} elsif ($pixels->[$i_left]->[2] == 0 && $pixels->[$i]->[2] > 0 && $pixels->[$i_right]->[2] == 0) {
			$gen->[$i]->[2] = $max_brightness if ($blue_rule & (1 << 2));
		} elsif ($pixels->[$i_left]->[2] == 0 && $pixels->[$i]->[2] > 0 && $pixels->[$i_right]->[2] > 0) {
			$gen->[$i]->[2] = $max_brightness if ($blue_rule & (1 << 3));
		} elsif ($pixels->[$i_left]->[2] > 0 && $pixels->[$i]->[2] == 0 && $pixels->[$i_right]->[2] == 0) {
			$gen->[$i]->[2] = $max_brightness if ($blue_rule & (1 << 4));
		} elsif ($pixels->[$i_left]->[2] > 0 && $pixels->[$i]->[2] == 0 && $pixels->[$i_right]->[2] > 0) {
			$gen->[$i]->[2] = $max_brightness if ($blue_rule & (1 << 5));
		} elsif ($pixels->[$i_left]->[2] > 0 && $pixels->[$i]->[2] > 0 && $pixels->[$i_right]->[2] == 0) {
			$gen->[$i]->[2] = $max_brightness if ($blue_rule & (1 << 6));
		} elsif ($pixels->[$i_left]->[2] > 0 && $pixels->[$i]->[2] > 0 && $pixels->[$i_right]->[2] > 0) {
			$gen->[$i]->[2] = $max_brightness if ($blue_rule & (1 << 7));
		}
		
	}

	$pixels = $gen;
	$client->put_pixels(0,$pixels);
 	sleep 1;
}

print "Done\n";
