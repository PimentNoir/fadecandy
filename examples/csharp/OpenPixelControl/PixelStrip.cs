using System;
using System.Collections.Generic;

namespace OpenPixelControl
{
    public class PixelStrip : LinkedList<Pixel>
    {
        private readonly object syncObject = new object();

        public int Size { get; private set; }

        public PixelStrip(int size)
        {
            Size = size;
            for (int i = 0; i < Size; i++)
            {
                AddFirst(200, 200, 200);
            }
        }

        public void AddFirst(byte obj, byte obj2, byte obj3)
        {
            base.AddFirst(new Pixel(obj, obj2, obj3));
            lock (syncObject)
            {
                while (base.Count > Size)
                {
                    base.RemoveLast();
                }
            }
        }

        public void AddLast(byte obj, byte obj2, byte obj3)
        {
            base.AddLast(new Pixel(obj, obj2, obj3));
            lock (syncObject)
            {
                while (base.Count > Size)
                {
                    base.RemoveFirst();
                }
            }
        }


    }

    public class Pixel : Tuple<byte, byte, byte>
    {
        public byte r;
        public byte g;
        public byte b;

        public Pixel(byte red, byte green, byte blue) : base(red, green, blue)
        {
            r = red;
            g = green;
            b = blue;
        }
    }
}
