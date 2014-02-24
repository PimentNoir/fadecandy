/*
 * Simple Open Pixel Control client for Node.js
 *
 * 2013-2014 Micah Elizabeth Scott
 * This file is released into the public domain.
 */

var net = require('net');
var fs = require('fs');

var OPC = function(host, port)
{
    this.host = host;
    this.port = port;
    this.pixelBuffer = null;
};

OPC.prototype.loadModel = function(filename)
{
    // Synchronously load a JSON model from a file on disk
    return JSON.parse(fs.readFileSync(filename))
}

OPC.prototype._reconnect = function()
{
    var _this = this;

    this.socket = new net.Socket()
    this.connected = false;

    this.socket.onclose = function() {
        console.log("Connection closed");
        _this.socket = null;
        _this.connected = false;
    }

    this.socket.connect(this.port, this.host, function() {
        console.log("Connected to " + _this.socket.remoteAddress);
        _this.connected = true;
        _this.socket.setNoDelay();
    });
}

OPC.prototype.writePixels = function()
{
    if (!this.socket) {
        this._reconnect();
    }
    if (!this.connected) {
        return;
    }
    this.socket.write(this.pixelBuffer);
}

OPC.prototype.setPixelCount = function(num)
{
    var length = 4 + num*3;
    if (this.pixelBuffer == null || this.pixelBuffer.length != length) {
        this.pixelBuffer = new Buffer(length);
    }

    // Initialize OPC header
    this.pixelBuffer.writeUInt8(0, 0);           // Channel
    this.pixelBuffer.writeUInt8(0, 1);           // Command
    this.pixelBuffer.writeUInt16BE(num * 3, 2);  // Length
}

OPC.prototype.setPixel = function(num, r, g, b)
{
    var offset = 4 + num*3;
    if (this.pixelBuffer == null || offset + 3 > this.pixelBuffer.length) {
        this.setPixelCount(num + 1);
    }

    this.pixelBuffer.writeUInt8(Math.max(0, Math.min(255, r | 0)), offset);
    this.pixelBuffer.writeUInt8(Math.max(0, Math.min(255, g | 0)), offset + 1);
    this.pixelBuffer.writeUInt8(Math.max(0, Math.min(255, b | 0)), offset + 2);
}

OPC.prototype.mapPixels = function(fn, model)
{
    // Set all pixels, by mapping each element of "model" through "fn" and setting the
    // corresponding pixel value. The function returns a tuple of three 8-bit RGB values.
    // Implies 'writePixels' as well. Has no effect if the OPC client is disconnected.

    if (!this.socket) {
        this._reconnect();
    }
    if (!this.connected) {
        return;
    }

    this.setPixelCount(model.length);
    var offset = 4;

    for (var i = 0; i < model.length; i++) {
        var rgb = fn(model[i]);

        this.pixelBuffer.writeUInt8(Math.max(0, Math.min(255, rgb[0] | 0)), offset);
        this.pixelBuffer.writeUInt8(Math.max(0, Math.min(255, rgb[1] | 0)), offset + 1);
        this.pixelBuffer.writeUInt8(Math.max(0, Math.min(255, rgb[2] | 0)), offset + 2);
        offset += 3;
    }

    this.writePixels();
}

OPC.prototype.mapParticles = function(particles, model)
{
    // Set all pixels, by mapping a particle system to each element of "model".
    // The particles include parameters 'point', 'intensity', 'falloff', and 'color'.

    function shader(p) {
        var r = 0;
        var g = 0;
        var b = 0;

        for (var i = 0; i < particles.length; i++) {
            var particle = particles[i];


            // Particle to sample distance
            var dx = (p.point[0] - particle.point[0]) || 0;
            var dy = (p.point[1] - particle.point[1]) || 0;
            var dz = (p.point[2] - particle.point[2]) || 0;
            var dist2 = dx * dx + dy * dy + dz * dz;

            // Particle edge falloff
            var intensity = particle.intensity / (1 + particle.falloff * dist2);

            // Intensity scaling
            r += particle.color[0] * intensity;
            g += particle.color[1] * intensity;
            b += particle.color[2] * intensity;
        }

        return [r, g, b];
    }

    this.mapPixels(shader, model);
}

module.exports = OPC;
