/*
 * Object for keeping track of the layout of our triangular grid.
 * The triangle is made of cells, which have information about their
 * connectedness to nearby cells.
 */

public class TriangleGrid
{
  Cell[] cells;
  
  class Cell
  {
    PVector center;
    int[] neighbors;

    Cell(float cx, float cy, int n1, int n2, int n3)
    {
      this.center = new PVector(cx, cy);
      this.neighbors = new int[3];
      this.neighbors[0] = n1;
      this.neighbors[1] = n2;
      this.neighbors[2] = n3;
    }
  };

  void grid16()
  {
    // Layout for a 16-cell triangular grid.

    // Each triangle side is 1 unit. "h" is the triangle height
    float h = sin(radians(60));

    cells = new Cell[16];

    // Bottom row, left to right 
    cells[ 0] = new Cell( 0.0, h*0 + h*1/3,  -1,  1, -1 ); 
    cells[ 1] = new Cell( 0.5, h*0 + h*2/3,  11,  2,  0 ); 
    cells[ 2] = new Cell( 1.0, h*0 + h*1/3,  -1,  3,  1 ); 
    cells[ 3] = new Cell( 1.5, h*0 + h*2/3,   9,  4,  2 ); 
    cells[ 4] = new Cell( 2.0, h*0 + h*1/3,  -1,  5,  3 ); 
    cells[ 5] = new Cell( 2.5, h*0 + h*2/3,   7,  6,  4 ); 
    cells[ 6] = new Cell( 3.0, h*0 + h*1/3,  -1, -1,  5 ); 

    // Second row, right to left
    cells[ 7] = new Cell( 2.5, h*1 + h*1/3,   5,  8, -1 ); 
    cells[ 8] = new Cell( 2.0, h*1 + h*2/3,  12,  9,  7 ); 
    cells[ 9] = new Cell( 1.5, h*1 + h*1/3,   3, 10,  8 ); 
    cells[10] = new Cell( 1.0, h*1 + h*2/3,  14, 11,  9 ); 
    cells[11] = new Cell( 0.5, h*1 + h*1/3,   1, -1, 10 ); 

    // Third row, left to right
    cells[12] = new Cell( 1.0, h*2 + h*1/3,   8, 13, -1 ); 
    cells[13] = new Cell( 1.5, h*2 + h*2/3,  15, 14, 13 ); 
    cells[14] = new Cell( 2.0, h*2 + h*1/3,  10, -1, 14 ); 

    // Top
    cells[15] = new Cell( 1.5, h*3 + h*1/3,  13, -1, -1 );     

    // Move the centroid to the origin
    translate(-1.5, -h*4/3);
  }

  void leds(OPC opc, int index)
  {
    // Create LED mappings, using the current grid coordinates
    for (int i = 0; i < cells.length; i++) {
      opc.led(index + i, int(cells[i].center.x + 0.5), int(cells[i].center.y + 0.5));
    }
  }
  
  void translate(float x, float y)
  {
    // Translate all points by this amount
    PVector t = new PVector(x, y);
    for (int i = 0; i < cells.length; i++) {
      cells[i].center.add(t);
    }
  }
  
  void mirror()
  {
    // Mirror all points left-to-right
    for (int i = 0; i < cells.length; i++) {
      cells[i].center.x = -cells[i].center.x;
    }
  }
  
  void scale(float s)
  {
    // Scale all points by this amount
    for (int i = 0; i < cells.length; i++) {
      cells[i].center.mult(s);
    }
  }
 
  void rotate(float angle)
  {
    // Rotate all points around the origin by this angle, in radians
    for (int i = 0; i < cells.length; i++) {
      cells[i].center.rotate(angle);
    }
  }
};
