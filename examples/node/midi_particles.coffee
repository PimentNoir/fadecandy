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
origin = [0, 0, 0]

# Physics
numTimesteps = 20
gain = 0.005

previousNow = 0
spinAngle = 0

# Time clock in seconds
clock = () -> 0.001 * new Date().getTime()

# Midi to frequency
midiToHz = (key) -> 440 * Math.pow 2, (key - 69) / 12

# Midi note to angle, one rev per octave
midiToAngle = (key) -> (2 * Math.PI / 24) * key


input.on 'message', (deltaTime, message) ->
    console.log message

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
                timestamp: clock()

            # Split keyboard into particles and LFOs
            if key >= 60
                particleNotes[key] = info
            else
                lfoNotes[key] = info

        when 0xb0  # Voice 0, Control Change
            switch message[1]
                when 7   # "Data entry" slider, brightness
                    brightness = message[2] * 2.0 / 127

                when 1   # "Modulation" slider, particle speed
                    particleLifetime = 0.1 + message[2] * 2.0 / 127

        when 0xe0  # Voice 0, Pitch Bend
            # Default spin 1.0, but allow forward/backward
            spinRate = 1.0 + (message[2] - 64) * 20.0 / 64


draw = () ->

    # Time delta calculations
    now = clock()
    timeStep = now - previousNow
    previousNow = now

    # Global spin update
    spinAngle = (spinAngle + timeStep * spinRate) % (Math.PI * 2)

    # Launch new particles for all active notes
    for key, note of particleNotes
        particles.push
            life: 1
            note: note
            point: origin.slice 0
            velocity: [0, 0, 0]
            timestamp: note.timestamp

    # Update appearance of all particles
    for p in particles

        # Angle: Global spin, thne positional mapping to key
        theta = midiToAngle p.note.key

        # Radius: Particles spawn in center, fly outward
        radius = 3.0 * (1 - p.life)

        # Positioned in polar coordinates
        x = radius * Math.cos theta
        y = radius * Math.sin theta

        # One rainbow per octave
        hue = (p.note.key - 60 + 0.1) / 12.0
        p.color = OPC.hsv hue, 0.5, 0.8

        # Intensity mapped to velocity, nonlinear
        p.intensity = Math.pow(p.note.velocity / 100, 2.0) * 0.2 * brightness

        # Falloff gets sharper as the note gets higher
        p.falloff = 15 * Math.pow(2, (p.note.key - 60) / 6)

        # Add influence of LFOs
        for key, note of lfoNotes
            age = now - note.timestamp
            hz = midiToHz key
            lfoAngle = midiToAngle key

            # Amplitude starts with left hand velocity
            wobbleAmp = Math.pow(note.velocity / 100, 2.0) * 12.0

            # Scale based on particle fuzziness
            wobbleAmp /= p.falloff

            # Fade over time
            wobbleAmp /= 1 + age

            # Wobble
            wobbleAmp *= Math.sin(p.life * Math.pow(3, (p.note.key - 35) / 12.0))

            # Wobble angle driven by LFO note and particle life
            x += wobbleAmp * Math.cos lfoAngle
            y += wobbleAmp * Math.sin lfoAngle

        # Update velocity; use the XZ plane
        p.velocity[0] += (x + origin[0] - p.point[0]) * gain
        p.velocity[2] += (y + origin[2] - p.point[2]) * gain

        # Fixed timestep physics
        for i in [1 .. numTimesteps]
            p.point[0] += p.velocity[0]
            p.point[1] += p.velocity[1]
            p.point[2] += p.velocity[2]

        p.life -= timeStep / particleLifetime

    # Filter out dead particles
    particles = particles.filter (p) -> p.life > 0

    # Render particles to the LEDs
    client.mapParticles particles, model

setInterval draw, 5
