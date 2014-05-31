#!/usr/bin/perl -w

use strict;
use IO::Socket;
use Data::Dumper;

$Data::Dumper::Indent = 0;

package OPC;

###
# &new
# Class instantiator
# Takes:
#    $server, a colon-separated list of the host and port on which an OPC server runs
#    $long_connection (optional) - maintain a stateful connection to the server. Default 1.
#    $verbose (optional) - whether or not debug information will be prnited to STDOUT.
# Return $client, a functioning OPC client object
# Example:
#    my $client = new OPC('localhost:7890');
###
sub new {
	my ($self, $server, $long_connection, $verbose) = @_;
	
	$long_connection ||= 1;
	$verbose ||= 0;
	
	my ($host, $port) = split (/:/,$server);

	my $this = {
		server => $server,
		long_connection => $long_connection,
		verbose => $verbose,
		host => $host,
		port => $port,
		socket => undef
	};
	bless $this;
	
	if ($verbose){
		print "Initializing OPC client on $server";
		print " with stateful connections enabled" if $long_connection;
		print ".\n";
	}
	
	return $this;
}

###
# &debug
# Prints debug information if we're operating in verbose mode
# Takes:
#    $self - a bless OPC object
#    $message - the message to be printed
# Returns nothing of value
###
sub debug {
	my ($self, $message) = @_;
	if ($self->{verbose}){
		print "$message\n";
	}
}

###
# &_ensure_connected
# Sets up a connection to the OPC server if one doesn't already exist.
# Takes: $self, a blesed OPC object
# Returns 1 on success and 0 on failure
###
sub _ensure_connected {
	my ($self) = @_;
	my $status = 0;
	$self->debug("_ensure_connected: called");
	if ($self->{socket}){
		$self->debug("_ensure_connected: already connected, doing nothing");
		$status = 1;
	} else {
		$self->{socket} = new IO::Socket::INET (
			PeerAddr => $self->{host},
			PeerPort => $self->{port},
			Proto => 'tcp'
		);
		if ($self->{socket}){
			$self->debug("_ensure_connected: ...success!");
			$status = 1;
		} else {
			$self->debug("_ensure_connected: ...fail!\n$!");
			$status = 0;
		}
	}
	$self->debug("_ensure_connected: returning $status");
	return $status;
}

###
# &_disconnect
# Drops the connection to the server if there is one.
# Takes: $self, a blessed OPC object
# Returns nothing of value
###
sub disconnect {
	my ($self) = @_;
	my $status = 0;
	$self->debug("disconnect: called");
	if ($self->{socket}){
		$self->debug("disconnect: socket found. shutting down.");
		$self->{socket}->shutdown();
		$self->{socket} = undef;
		$status = 1;
	} else {
		$self->debug("disconnect: no socket found; no action taken.");
		$status = 1;
	}
	$self->debug("disconnect: returning $status");
	return $status;
}

###
# &can_connect
# Attempts to connect to the server.
# Takes $self, a blessed OPC object
# Returns 1 on success, 0 on failure
# If operating in long_connection mode (see &new), this connection will be 
# reused for subsequent put_pixels calls
###
sub can_connect{
	my ($self) = @_;
	my $status = 0;
	$self->debug("can_connect: called");
	$status = $self->_ensure_connected();
	unless ($self->{long_connection}){
		$self->disconnect();
	}
	$self->debug("can_connect: returning $status");
	return $status;
}

###
# &put_pixels
# Send the list of pixel colors to the OPC server on the given channel.
# Takes:
#    $self - A blessed OPC object
#    $channel - Which strand of lights to send the pixel colors to.
#               Must be an integer in the range 0-255, inclusive.
#               0 is a special value meaning "all channels"
#    $pixels - An array reference of 3-tuple array references of RGP folors
#              Each value should be in the range 0-255, inclusive.
#              Floats will be rounded to integers. Values outside range will be trimmed.
# Returns 1 on success, 0 on failure
# Will establish a connection to the server as needed
# Example:
#    $client->put_pixels(0,[[255,255,255],[0,0,0],[255,255,255]]);
###
sub put_pixels {
	my ($self, $channel, $pixels) = @_;
	my $status = 0;
	$self->debug("put_pixels: called with channel $channel and pixels ".Data::Dumper::Dumper($pixels));

	my $is_connected = $self->_ensure_connected();
	if ($is_connected){
		
		my $len_hi_byte = int(scalar(@$pixels)*3 / 256);
		my $len_lo_byte = (scalar(@$pixels)*3) % 256;
		my $header = chr($channel) . chr(0) . chr($len_hi_byte) . chr($len_lo_byte);
		my $pieces = [$header];
		foreach my $pixel (@$pixels) {
			
			# Make sure we have three subpixels: not two, and not four. Five is right out.
			push (@$pixel, 0) while (scalar(@$pixel) < 3);
			pop (@$pixel) while (scalar(@$pixel) > 3);
			
			# Ensure each pixel is in the range 0-255, inclusive
			foreach my $pos (0..scalar(@$pixel)-1){
				$pixel->[$pos] = int($pixel->[$pos] || 0);
				$pixel->[$pos] = 0 if $pixel->[$pos] < 0;
				$pixel->[$pos] = 255 if $pixel->[$pos] > 255;
			}
			
			# Append these pixels to the message array
			push(@$pieces, chr($pixel->[0]) . chr($pixel->[1]) . chr($pixel->[2]));
		}
		my $message = join('', @$pieces);
		$self->debug('put_pixels: Sending pixels to the server:');
		$self->debug('put_pixels: '.join(" ",map({sprintf("%x",ord($_))} split(//,$message))));
		
		$self->{socket}->send($message);
		
		if ($!){
			$self->debug('put_pixels: Received socket error: $!');
			$status = 0;
		} else {
			$status = 1;
		}
		
		unless ($self->{long_connection}) {
			$self->debug('put_pixels: not in long_connection / stateful mode. Disconnecting');
			$self->disconnect();
		}
	} else {
		$self->debug('put_pixels: not connected. Ignoring these pixels.');
		$status = 0;
	}

	$self->debug("put_pixels: returning $status");
	return $status;
}

1;