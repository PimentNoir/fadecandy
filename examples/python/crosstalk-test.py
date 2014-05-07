#!/usr/bin/env python

# Open Pixel Control client: Test crosstalk between LED strips;
# send each strip a different pattern, use a lot of low-brightness
# pixels so that glitches show up clearly.
#
# This also helps identify strips. The first three LEDs are colored
# according to the strip number, in binary: MSB first, bright green
# for 1 and dim red for 0.

import opc, time

client = opc.Client('localhost:7890')

bits = ( (80,0,0), (0,255,0) )

while True:
	# Flash each strip in turn
	for strip in range(8):
		pixels = [ (90,90,90) ] * 512
		for i in range(32):
			pixels[strip * 64 + i * 2] = (200,200,200)

		# Label all strips always
		for s in range(8):
			pixels[s * 64 + 0] = bits[(s >> 2) & 1]
			pixels[s * 64 + 1] = bits[(s >> 1) & 1]
			pixels[s * 64 + 2] = bits[(s >> 0) & 1]

		client.put_pixels(pixels)
		time.sleep(0.5)
