#!/usr/bin/perl -w

use strict;
use OPC;
use Time::HiRes qw/usleep/;

my $num_leds = 64;
my $client = new OPC('localhost:7890');
$client->can_connect();

my $i = 0;

while(1){
	my $pixels = [];
	
	# Initialize an empty pixel array
	push @$pixels, [0,0,0] while scalar(@$pixels) < $num_leds;

	# Set the Ith (mod num_leds) pixel to blinding bright white
	$pixels->[$i % $num_leds] = [255,255,255];

	# Send this row of pixels to the server
	$client->put_pixels(0,$pixels);

	usleep 5000;
	
	$i++;
}

print "Done\n";