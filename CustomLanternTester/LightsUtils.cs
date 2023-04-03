using System;

namespace CustomLanternTester
{
    internal static class LightsUtils
    {
        // https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
        // https://gist.github.com/ibober/6b5a6e1dea888c01c0af175e71b15fa4
        public static int[] GetRgbFromTemperature(double temperature)
        {
            // Temperature must fit between 1000 and 40000 degrees.
            temperature = MathUtils.Clamp(temperature, 1000, 40000);

            // All calculations require temperature / 100, so only do the conversion once.
            temperature /= 100;

            // Compute each color in turn.
            int red, green, blue;

            // First: red.
            if (temperature <= 66)
            {
                red = 255;
            }
            else
            {
                // Note: the R-squared value for this approximation is 0.988.
                red = (int)(329.698727446 * (Math.Pow(temperature - 60, -0.1332047592)));
                red = MathUtils.Clamp(red, 0, 255);
            }

            // Second: green.
            if (temperature <= 66)
            {
                // Note: the R-squared value for this approximation is 0.996.
                green = (int)(99.4708025861 * Math.Log(temperature) - 161.1195681661);
            }
            else
            {
                // Note: the R-squared value for this approximation is 0.987.
                green = (int)(288.1221695283 * (Math.Pow(temperature - 60, -0.0755148492)));
            }

            green = MathUtils.Clamp(green, 0, 255);

            // Third: blue.
            if (temperature >= 66)
            {
                blue = 255;
            }
            else if (temperature <= 19)
            {
                blue = 0;
            }
            else
            {
                // Note: the R-squared value for this approximation is 0.998.
                blue = (int)(138.5177312231 * Math.Log(temperature - 10) - 305.0447927307);
                blue = MathUtils.Clamp(blue, 0, 255);
            }

            return new[] { red, green, blue };
        }
    }
}
