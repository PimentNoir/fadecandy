#!/usr/bin/perl -w

use strict;
use OPC;

my $num_leds = 64;
my $client = new OPC('localhost:7890');
$client->can_connect();

my $red_pos = 0;
my $blue_pos = 0;
my $green_pos = 0;

while(1){
	my $pixels = [];
	
	# Initialize an empty pixel array
	push @$pixels, [0,0,0] while scalar(@$pixels) < $num_leds;

	# Set the red color blocks
	foreach my $pos ($red_pos..($red_pos+($num_leds / 2))){
		my $offset = $pos - $red_pos;
		$pos -= $num_leds if $pos > $num_leds;
		$pixels->[$pos]->[0] = int(255 * sin($offset / ($num_leds / 2) * 3.14159) );
	}

	# Set the green color blocks
	foreach my $pos ($green_pos..($green_pos+($num_leds / 2))){
		my $offset = $pos - $green_pos;
		$pos -= $num_leds if $pos > $num_leds;
		$pixels->[$pos]->[1] = int(255 * sin($offset / ($num_leds / 2) * 3.14159) ); 
	}

	# Set the blue color blocks
	foreach my $pos ($blue_pos..($blue_pos+($num_leds / 2))){
		my $offset = $pos - $blue_pos;
		$pos -= $num_leds if $pos > $num_leds;
		$pixels->[$pos]->[2] = int(255 * sin($offset / ($num_leds / 2) * 3.14159) ); 
	}

	# Send this row of pixels to the server
	$client->put_pixels(0,$pixels);

	sleep 1;

	# Move the color blocks forward at different rates
	$red_pos += 10;
	$green_pos += 5;
	$blue_pos += 1;
	$red_pos -= $num_leds if $red_pos > $num_leds;
	$green_pos -= $num_leds if $green_pos > $num_leds;
	$blue_pos -= $num_leds if $blue_pos > $num_leds;
}

print "Done\n";