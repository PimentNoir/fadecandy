#!/usr/bin/env coffee
#
# Particle system, playable with a MIDI keyboard!
#
# Dependencies:
#    npm install midi coffee
#
# Assumes the default MIDI input port (0).
# Specify a layout file on the command line, or use the default (grid32x16z)
#
# Controls:
#    (Tested with Alesis Q49 keyboard controller)
#    Left hand  -> Low frequency oscillators (wobble)
#    Right hand -> Particles, shifting in color and pointiness
#
# 2014, Micah Elizabeth Scott & Keroserene
#

# Default MIDI input
midi = require 'midi'
input = new midi.input
input.openPort 0
input.ignoreTypes false, false, false

# Default OPC output
OPC = new require './opc'
model = OPC.loadModel process.argv[2] || '../layouts/grid32x16z.json'
client = new OPC 'localhost', 7890

# Live particles
particles = []

# Notes for low frequency oscillators
lfoNotes = {}

# Notes for particles
particleNotes = {}

# Adjustable parameters
particleLifetime = 1.0
brightness = 1.0
spinRate = 1.0

midiTime = 0
previousNow = 0
spinAngle = 0

input.on 'message', (deltaTime, message) ->

    # Keep time
    midiTime += deltaTime

    switch message[0]
        when 0x80  # Voice 0, note off
            key = message[1]
            delete lfoNotes[key]
            delete particleNotes[key]

        when 0x90  # Voice 0, note on
            key = message[1]
            info =
                key: key
                velocity: message[2]
                timestamp: midiTime

            # Split keyboard into particles and LFOs
            if key >= 60
                particleNotes[key] = info
            else
                lfoNotes[key] = info

        when 0xb0  # Voice 0, Control Change
            switch message[1]
                when 7   # "Data entry" slider, brightness
                    brightness = message[2] * 3.0 / 127

                when 1   # "Modulation" slider, particle speed
                    particleLifetime = 0.1 + message[2] * 2.0 / 127

        when 0xe0  # Voice 0, Pitch Bend
            # Default spin 1.0, but allow forward/backward
            spinRate = 1.0 + (message[2] - 64) * 20.0 / 64


draw = () ->

    # Time delta calculations
    now = 0.001 * new Date().getTime()
    timeStep = now - previousNow
    previousNow = now

    # Global spin update
    spinAngle = (spinAngle + timeStep * spinRate) % (Math.PI * 2)

    # Launch new particles for all active notes
    for key, note of particleNotes
        particles.push
            life: 1
            note: note
            timestamp: note.timestamp

    # Update appearance of all particles
    for p in particles

        # Angle: Global spin, thne positional mapping to key
        theta = spinAngle + (Math.PI / 5) * p.note.key

        # Positioned in polar coordinates, on unit circle
        x = Math.cos theta
        y = Math.sin theta

        # Add influence of LFOs
        for key, note of lfoNotes

            # Down several octaves, to useful LFO frequencies
            transpose = -12 * 5

            # Midi note to frequency
            hz = 440 * Math.pow 2, (note.key - 69 + transpose) / 12

            # Wobble amplitude driven by LFO
            wobbleAmp = Math.sin Math.PI * 2 * now * hz

            # Wobble angle driven by LFO note and particle life
            wobbleAngle = 10.0 * p.life + (Math.PI / 5) * p.note.key

            x += wobbleAmp * Math.cos wobbleAngle
            y += wobbleAmp * Math.sin wobbleAngle

        # Radius: Particles spawn in center, fly outward
        radius = 3.0 * (1 - p.life)
        x *= radius
        y *= radius

        # Use the XZ plane
        p.point = [x, 0, y]

        # One rainbow per octave
        hue = (p.note.key - 60) / 12.0
        p.color = OPC.hsv hue, 0.5, 0.8

        # Intensity mapped to velocity, nonlinear
        p.intensity = Math.pow(p.note.velocity / 100, 5.0) * brightness

        # Falloff gets sharper as the note gets higher
        p.falloff = 20 + (p.note.key - 60) * 20

        p.life -= timeStep / particleLifetime

    # Filter out dead particles
    particles = particles.filter (p) -> p.life > 0

    # Render particles to the LEDs
    client.mapParticles particles, model

setInterval draw, 10
