/*
 * Simple Open Pixel Control client for Processing.
 * This version is for lower-level control, without any automatic frame sampling.
 *
 * Micah Elizabeth Scott, 2013
 * This file is released into the public domain.
 */

import java.net.*;

public class OPCLowLevel
{
  Socket socket;
  OutputStream output;
  String host;
  int port;

  byte[] packetData;
  byte firmwareConfig;
  String colorCorrection;

  OPCLowLevel(String host, int port, int numPixels)
  {
    this.host = host;
    this.port = port;

    int numBytes = 3 * numPixels;
    int packetLen = 4 + numBytes;

    packetData = new byte[packetLen];
    packetData[0] = 0;  // Channel
    packetData[1] = 0;  // Command (Set pixel colors)
    packetData[2] = (byte)(numBytes >> 8);
    packetData[3] = (byte)(numBytes & 0xFF);
  }

  void setPixel(int index, int r, int g, int b)
  {
    int addr = 4 + index * 3;
    packetData[addr] = (byte)r;
    packetData[addr+1] = (byte)g;
    packetData[addr+2] = (byte)b;
  }

  // Send a packet with the current firmware configuration settings
  void sendFirmwareConfigPacket()
  {
    if (output == null) {
      // We'll do this when we reconnect
      return;
    }
 
    byte[] packet = new byte[9];
    packet[0] = 0;          // Channel (reserved)
    packet[1] = (byte)0xFF; // Command (System Exclusive)
    packet[2] = 0;          // Length high byte
    packet[3] = 5;          // Length low byte
    packet[4] = 0x00;       // System ID high byte
    packet[5] = 0x01;       // System ID low byte
    packet[6] = 0x00;       // Command ID high byte
    packet[7] = 0x02;       // Command ID low byte
    packet[8] = firmwareConfig;

    try {
      output.write(packet);
    } catch (Exception e) {
      dispose();
    }
  }

  // Send a packet with the current color correction settings
  void sendColorCorrectionPacket()
  {
    if (colorCorrection == null) {
      // No color correction defined
      return;
    }
    if (output == null) {
      // We'll do this when we reconnect
      return;
    }

    byte[] content = colorCorrection.getBytes();
    int packetLen = content.length + 4;
    byte[] header = new byte[8];
    header[0] = 0;          // Channel (reserved)
    header[1] = (byte)0xFF; // Command (System Exclusive)
    header[2] = (byte)(packetLen >> 8);
    header[3] = (byte)(packetLen & 0xFF);
    header[4] = 0x00;       // System ID high byte
    header[5] = 0x01;       // System ID low byte
    header[6] = 0x00;       // Command ID high byte
    header[7] = 0x01;       // Command ID low byte

    try {
      output.write(header);
      output.write(content);
    } catch (Exception e) {
      dispose();
    }
  }

  void sendPixels()
  {
    if (output == null) {
      // Try to (re)connect
      connect();
    }
    if (output == null) {
      return;
    }

    try {
      output.write(packetData);
    } catch (Exception e) {
      dispose();
    }
  }

  void dispose()
  {
    // Destroy the socket. Called internally when we've disconnected.
    if (output != null) {
      println("Disconnected from OPC server");
    }
    socket = null;
    output = null;
  }

  void connect()
  {
    // Try to connect to the OPC server. This normally happens automatically in draw()
    try {
      socket = new Socket(host, port);
      socket.setTcpNoDelay(true);
      output = socket.getOutputStream();
      println("Connected to OPC server");
    } catch (ConnectException e) {
      dispose();
    } catch (IOException e) {
      dispose();
    }
    
    sendColorCorrectionPacket();
    sendFirmwareConfigPacket();
  }
}

