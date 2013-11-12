#!/usr/bin/env python

# Open Pixel Control client: Every other light to solid white, others dark.

import opc, time

numPairs = 256
client = opc.Client('localhost:7890')

black = [ (0,0,0), (0,0,0) ] * numPairs
white = [ (255,255,255), (0,0,0) ] * numPairs

# Fade to white
client.put_pixels(black)
client.put_pixels(black)
time.sleep(0.5)
client.put_pixels(white)
