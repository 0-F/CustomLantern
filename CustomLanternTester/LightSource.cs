using System;
using System.Globalization;

namespace CustomLanternTester
{
    internal class LightSource
    {
        public LightSource(
            string name,
            string? red, string? green, string? blue, string? alpha,
            string? spRed, string? spGreen, string? spBlue, string? spAlpha,
            string? x, string? y, string? z,
            string? radius, string? intensity,
            string? comment)
        {
            CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;

            Name = name;
            Red = red;
            Green = green;
            Blue = blue;
            Alpha = alpha;
            SpRed = spRed;
            SpGreen = spGreen;
            SpBlue = spBlue;
            SpAlpha = spAlpha;
            X = x;
            Y = y;
            Z = z;
            Radius = radius;
            Intensity = intensity;
            Comment = comment;

            if (red != null && green != null && blue != null)
            {
                string tmpAlpha = alpha ?? "1";

                // #AARRGGBB
                HexColor = "#" +
                    ((byte)(Convert.ToSingle(tmpAlpha) * 255)).ToString("X2") +
                    ((byte)(Convert.ToSingle(red) * 255)).ToString("X2") +
                    ((byte)(Convert.ToSingle(green) * 255)).ToString("X2") +
                    ((byte)(Convert.ToSingle(blue) * 255)).ToString("X2");

            }
            else
            {
                HexColor = "#00000000";
            }

            if (spRed != null && spGreen != null && spBlue != null)
            {
                string tmpSpAlpha = alpha ?? "1";

                // #AARRGGBB
                HexSpColor = "#" +
                    ((byte)(Convert.ToSingle(tmpSpAlpha) * 255)).ToString("X2") +
                    ((byte)(Convert.ToSingle(spRed) * 255)).ToString("X2") +
                    ((byte)(Convert.ToSingle(spGreen) * 255)).ToString("X2") +
                    ((byte)(Convert.ToSingle(spBlue) * 255)).ToString("X2");
            }
            else
            {
                HexSpColor = "#00000000";
            }
        }

        public string Name { get; }
        public string? Red { get; }
        public string? Green { get; }
        public string? Blue { get; }
        public string? Alpha { get; }
        public string? SpRed { get; }
        public string? SpGreen { get; }
        public string? SpBlue { get; }
        public string? SpAlpha { get; }
        public string? X { get; }
        public string? Y { get; }
        public string? Z { get; }
        public string? Radius { get; }
        public string? Intensity { get; }
        public string? Comment { get; }
        public string HexColor { get; } // #AARRGGBB
        public string HexSpColor { get; } // #AARRGGBB
    }
}
