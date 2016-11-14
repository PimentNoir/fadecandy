using System.Threading;

namespace OpenPixelControl
{
    public class Program
    {
        public static void Main(string[] args)
        {
            Client client = new Client("127.0.0.1", 7890, true, true);
            PixelStrip pixels = new PixelStrip(30);

            while (true)
            {
                //rainbow effect
                for (int i = 0; i <= 360; i = i + 5)
                {
                    var color = new HSLColor(hue: i, saturation: 100, luminosity: 100);
                    pixels.AddFirst(color.ToRgbPixel());
                    client.putPixels(pixels);
                    Thread.Sleep(100);
                }
            }
        }
    }
}
