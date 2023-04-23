using IniParser.Model;
using IniParser;
using SoulsFormats;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CustomLanternPatcher
{
    internal class LanternSettings
    {
        public float? red;
        public float? green;
        public float? blue;
        public float? alpha;
        public float? sp_red;
        public float? sp_green;
        public float? sp_blue;
        public float? sp_alpha;
        public float? x;
        public float? y;
        public float? z;
        public float? radius;
        public float? intensity;

        public LanternSettings(string file)
        {
            Load(file);
        }

        public static float? StringToSingle(string? str)
        {
            if (str == null) { return null; }

            return Convert.ToSingle(str);
        }

        public void Load(string file)
        {
            IniData data = new();
            var parser = new FileIniDataParser();

            // load user settings values if any
            if (File.Exists(file))
            {
                data = parser.ReadFile(file);
            }
            else
            {
                Console.Error.WriteLine($"Configuration file for the light lantern \"{file}\" does not exist.");
                Environment.Exit(1);
            }

            // get values
            var config = data["config"];
            red = StringToSingle(config["red"]);
            green = StringToSingle(config["green"]);
            blue = StringToSingle(config["blue"]);
            alpha = StringToSingle(config["alpha"]);
            sp_red = StringToSingle(config["sp_red"]);
            sp_green = StringToSingle(config["sp_green"]);
            sp_green = StringToSingle(config["sp_green"]);
            sp_blue = StringToSingle(config["sp_blue"]);
            sp_alpha = StringToSingle(config["sp_alpha"]);
            x = StringToSingle(config["x"]);
            y = StringToSingle(config["y"]);
            z = StringToSingle(config["z"]);
            radius = StringToSingle(config["radius"]);
            intensity = StringToSingle(config["intensity"]);
        }
    }
}
