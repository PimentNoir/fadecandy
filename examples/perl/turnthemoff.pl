#!/usr/bin/perl -w

use strict;
use OPC;

my $num_leds = 64;
my $client = new OPC('localhost:7890');
$client->can_connect();

my $pixels = [];
	
# Initialize an empty pixel array
push @$pixels, [0,0,0] while scalar(@$pixels) < $num_leds;

# Send this row of pixels to the server
$client->put_pixels(0,$pixels);
$client->put_pixels(0,$pixels);

print "Done\n";