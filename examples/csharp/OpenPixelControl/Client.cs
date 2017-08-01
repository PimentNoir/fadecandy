using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;

namespace OpenPixelControl
{
    public class RgbTupleList<T1, T2, T3> : List<Tuple<T1, T2, T3>>
    {
        public void Add(T1 item, T2 item2, T3 item3)
        {
            Add(new Tuple<T1, T2, T3>(item, item2, item3));
        }
    }

    public class Client : IDisposable
    {

        public bool _verbose;

        public bool _long_connection;

        public string _ip;

        public int _port;

        public Socket _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

        public Client(string ip, int port, bool long_connecton = true, bool verbose = false)
        {
            _ip = ip;
            _port = port;
            _long_connection = long_connecton;
            _verbose = verbose;

            debug(string.Format("{0}:{1}", _ip, _port));
        }

        private static void debug(string message)
        {
            Console.WriteLine(message);
        }

        private bool ensureConnected()
        {
            if (_socket.Connected)
            {
                debug("Ensure Connected: already connected, doing nothing");
                return true;
            }

            else
            {
                try
                {
                    debug("Ensure Connected: trying to connect...");
                    _socket.Ttl = 1;
                    IPAddress ip = IPAddress.Parse(_ip);
                    _socket.Connect(ip, _port);
                    debug("Ensure Connected: ....success");
                    return true;
                }

                catch (SocketException e)
                {
                    Console.WriteLine(e.Message);
                    return false;
                }
            }
        }

        public void Dispose()
        {
            debug("Disconnecting");
            if (_socket.Connected)
            {
                _socket.Dispose();
            }
        }

        private bool canConnect()
        {
            bool success = ensureConnected();
            if (!_long_connection)
            {
                Dispose();
            }
            return success;
        }

        public void putPixels(PixelStrip pixels, int channel = 0)
        {
            debug("put pixes: connecting");
            bool is_connected = ensureConnected();
            if (!is_connected)
            {
                debug("Put pixels not connected. Ignoring these pixels.");
            }

            int len_hi_byte = pixels.Count * 3 / 256;
            int len_low_byte = (pixels.Count * 3) % 256;

            List<byte> pieces = new List<byte>
            {
                Convert.ToByte(channel),
                Convert.ToByte(0),
                Convert.ToByte(len_hi_byte),
                Convert.ToByte(len_low_byte)
            };

            foreach (var item in pixels)
            {
                pieces.Add(item.r);
                pieces.Add(item.g);
                pieces.Add(item.b);
            }

            byte[] message = new byte[pieces.Count];

            for (int i = 0; i < pieces.Count; i++)
            {
                message[i] = pieces[i];
            }


            _socket.Send(message);
            _socket.Send(message);

        }
    }
}
