/*
 * Model creation script for a 16-pixel triangular grid made up
 * of strips in a zig-zag layout.
 *
 * Fields in each pixel's JSON layout:
 *
 *        point: A 3D vector, arbitrary coordinates. Scaled to look good
 *               in the Open Pixel Control "gl_server" pre-visualizer.
 *               This describes the centroid of the triangle cell.
 *
 *    neighbors: List of array indices for neighbors of this cell
 *
 * 2014 Micah Elizabeth Scott
 * This file is released into the public domain.
 */

// Each triangle side is 1 unit. "h" is the triangle height
var w = 1;
var h = Math.sin(60 * Math.PI / 180);

// Move the centroid to the origin
var x = -1.5;
var z = -h*4/3;

cells = []

// Bottom row, left to right 
cells[ 0] = { point:[ x + w*0.0, 0, z + h*0 + h*1/3 ],  neighbors: [      1     ] }; 
cells[ 1] = { point:[ x + w*0.5, 0, z + h*0 + h*2/3 ],  neighbors: [ 11,  2,  0 ] }; 
cells[ 2] = { point:[ x + w*1.0, 0, z + h*0 + h*1/3 ],  neighbors: [      3,  1 ] }; 
cells[ 3] = { point:[ x + w*1.5, 0, z + h*0 + h*2/3 ],  neighbors: [  9,  4,  2 ] }; 
cells[ 4] = { point:[ x + w*2.0, 0, z + h*0 + h*1/3 ],  neighbors: [      5,  3 ] }; 
cells[ 5] = { point:[ x + w*2.5, 0, z + h*0 + h*2/3 ],  neighbors: [  7,  6,  4 ] }; 
cells[ 6] = { point:[ x + w*3.0, 0, z + h*0 + h*1/3 ],  neighbors: [          5 ] }; 

// Second row, right to left
cells[ 7] = { point:[ x + w*2.5, 0, z + h*1 + h*1/3 ],  neighbors: [  5,  8     ] }; 
cells[ 8] = { point:[ x + w*2.0, 0, z + h*1 + h*2/3 ],  neighbors: [ 12,  9,  7 ] }; 
cells[ 9] = { point:[ x + w*1.5, 0, z + h*1 + h*1/3 ],  neighbors: [  3, 10,  8 ] }; 
cells[10] = { point:[ x + w*1.0, 0, z + h*1 + h*2/3 ],  neighbors: [ 14, 11,  9 ] }; 
cells[11] = { point:[ x + w*0.5, 0, z + h*1 + h*1/3 ],  neighbors: [  1,     10 ] }; 

// Third row, left to right
cells[12] = { point:[ x + w*1.0, 0, z + h*2 + h*1/3 ],  neighbors: [  8, 13     ] }; 
cells[13] = { point:[ x + w*1.5, 0, z + h*2 + h*2/3 ],  neighbors: [ 15, 14, 13 ] }; 
cells[14] = { point:[ x + w*2.0, 0, z + h*2 + h*1/3 ],  neighbors: [ 10,     14 ] }; 

// Top
cells[15] = { point:[ x + w*1.5, 0, z + h*3 + h*1/3 ],  neighbors: [ 13         ] };     


console.log(JSON.stringify(cells));
