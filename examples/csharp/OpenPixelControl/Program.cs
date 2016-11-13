using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace OpenPixelControl
{
    public class Program
    {
        public static void Main(string[] args)
        {
            Client client = new Client("127.0.0.1", 7890, true, true);
            PixelStrip pixels = new PixelStrip(30);
            client.putPixels(pixels);
        }
    }
}
