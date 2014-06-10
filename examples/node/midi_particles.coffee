#!/usr/bin/env coffee
#
# Particle system, playable with a MIDI keyboard!
#
# Dependencies: run `npm install` in this directory.
#
# Command line options:
#    
#    --midiport <int>       Use a different MIDI port. Defaults to zero.
#    --layout <file>        Pick a JSON layout file. Defaults to grid32x16z
#    --opchost <hostname>   Network host running the OPC server. Defaults to localhost.
#    --opcport <port>       Network port for the OPC server. Defaults to 7890.
#
# Controls:
#    Left hand  -> Low frequency oscillators (wobble)
#    Right hand -> Particles, shifting in color and pointiness
#
# Supported MIDI keyboards:
#    - Basic MIDI works for any keyboard
#    - Pitch bend / modulation / data knobs on Alesis Q49
#    - Data sliders on Oxygen 61
#
# 2014, Micah Elizabeth Scott & Keroserene
#

parseArgs = require 'minimist'
midi = require 'midi'
OPC = new require './opc'

argv = parseArgs process.argv.slice(2),
    default:
        layout: '../layouts/grid32x16z.json'
        midiport: 0
        opchost: 'localhost'
        opcport: 7890

input = new midi.input
if argv.midiport >= input.getPortCount()
    console.log "MIDI port #{ argv.midiport } isn't available!"
    process.exit 1

console.log "Using MIDI port #{ argv.midiport }: #{ input.getPortName argv.midiport }"
input.openPort argv.midiport

console.log "Using OPC server at #{ argv.opchost }:#{ argv.opcport }"
model = OPC.loadModel argv.layout
client = new OPC argv.opchost, argv.opcport

# Live particles
particles = []
maxParticles = 500

# Notes for low frequency oscillators
lfoNotes = {}

# Notes for particles
particleNotes = {}

# Adjustable parameters
particleLifetime = 1.0
brightness = 1.0
spinRate = 0
noteSustain = 1.6
wobbleAmount = 24.0
origin = [0, 0, 0]

# Physics
frameDelay = 10
timestepSize = 0.010
gain = 0.1

# Controlled by the Pitch Transpose Knob
spinAngle = 0

# Time clock in seconds
clock = () -> 0.001 * new Date().getTime()

# Midi to frequency
midiToHz = (key) -> 440 * Math.pow 2, (key - 69) / 12

# Midi note to angle, one rev per octave
midiToAngle = (key) -> (2 * Math.PI / 24) * key

# Musical Constants
# Boundary between the left-hand and right-hand patterns.
LIMINAL_KEY = 46
MAX_VELOCITY = 100
SHARP_NAMES = ['A', 'A#', 'B', 'C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#']
FLAT_NAMES = ['A', 'Bb', 'B', 'C', 'Db', 'D', 'Eb', 'E', 'F', 'Gb', 'G', 'Ab']
LOWEST_A = 1
HIGHEST_C = 124
getKeyName = (key) -> FLAT_NAMES[(key - LOWEST_A) % 12]

input.ignoreTypes false, false, false
input.on 'message', (deltaTime, message) ->
    logMsg = message
    msgType = Math.floor parseInt(message[0]) / 16  # Examine the 0xF0 byte.

    switch msgType
        when 0x8  # Voice 0, note off
            key = message[1]
            logMsg += ' : ' + getKeyName key
            delete lfoNotes[key]
            delete particleNotes[key]

        when 0x9  # Voice 0, note on
            key = message[1]
            logMsg += ' : ' + getKeyName key
            info =
                key: key
                velocity: message[2]
                timestamp: clock()

            # Split keyboard into particles and LFOs
            if key >= LIMINAL_KEY
                particleNotes[key] = info
            else
                lfoNotes[key] = info

        # when 0xb  # Voice 0, Control Change
        #     switch message[1]
        #         when 7, 33   # "Data entry" / "C1" slider, brightness
        #             brightness = message[2] * 2.0 / 127

        #         when 1, 34   # "Modulation" / "C2" slider, particle speed
        #             console.log message[2]
        #             particleLifetime = 0.1 + message[2] * 2.0 / 127

        # when 0xe  # Voice 0, Pitch Bend
        #     # Default spin 1.0, but allow forward/backward
        #     spinRate = (message[2] - 64) * 10.0 / 64
    console.log logMsg

draw = () ->

    # Time delta calculations
    now = clock()

    # Global spin update
    spinAngle = (spinAngle + timestepSize * spinRate) % (Math.PI * 2)

    # Experiment - moves the origin in a circle
    theta = now * 2.5
    origin[0] = 0.2 * Math.cos theta
    origin[2] = 0.2 * Math.sin theta

    # Launch new particles for all active notes
    for key, note of particleNotes
        particles.push
            life: 1
            note: note
            point: origin.slice 0
            velocity: [0, 0, 0]
            timestamp: note.timestamp

    # Discard particles if necessary
    if particles.length > maxParticles
        particles = particles.slice particles.length - maxParticles

    # Update appearance of all particles
    for p in particles

        # Angle: Global spin, thne positional mapping to key
        theta = spinAngle + midiToAngle p.note.key

        # Radius: Particles spawn in center, fly outward
        radius = 3.0 * (1 - p.life)

        # Positioned in polar coordinates
        x = origin[0] + radius * Math.cos(theta)
        y = origin[2] + radius * Math.sin(theta)

        # Hop around between almost-opposing colors, eventually going
        # around the rainbow. These ratios control what kinds of color
        # schemes we get for different chords. This operates by the circle of
        # fifths.
        hue = (p.note.key - LIMINAL_KEY + 0.1) * (7 / 12.0)
        p.color = OPC.hsv hue, 0.5, 0.8

        # Intensity mapped to velocity, nonlinear
        p.intensity = Math.pow(p.note.velocity / MAX_VELOCITY, 2.0) * 0.2 * brightness

        # Fade with age
        noteAge = now - p.note.timestamp
        p.intensity *= Math.max(0, 1 - (noteAge / noteSustain))

        # Falloff gets sharper as the note gets higher
        p.falloff = 12 * Math.pow(1.18, (p.note.key - LIMINAL_KEY) / 6)

        # Add influence of LFOs
        for key, note of lfoNotes
            lfoAge = now - note.timestamp
            hz = midiToHz key
            lfoAngle = midiToAngle key

            # Amplitude starts with left hand velocity
            wobbleAmp = Math.pow(note.velocity / MAX_VELOCITY, 2.0) * wobbleAmount

            # Scale based on particle fuzziness
            wobbleAmp /= p.falloff

            # Fade over time
            wobbleAmp /= 1 + lfoAge

            # Wobble
            wobbleAmp *= Math.sin(p.life * Math.pow(3, (p.note.key - LIMINAL_KEY/2) / 12.0))

            # Wobble angle driven by LFO note and particle life
            x += wobbleAmp * Math.cos lfoAngle
            y += wobbleAmp * Math.sin lfoAngle

        # Update velocity; use the XZ plane
        p.velocity[0] += (x - p.point[0]) * gain
        p.velocity[2] += (y - p.point[2]) * gain

        p.point[0] += p.velocity[0]
        p.point[1] += p.velocity[1]
        p.point[2] += p.velocity[2]

        p.life -= timestepSize / particleLifetime

    # Filter out dead particles
    particles = particles.filter (p) -> p.life > 0 && p.intensity > 0

    # Render particles to the LEDs
    client.mapParticles particles, model

setInterval draw, frameDelay
