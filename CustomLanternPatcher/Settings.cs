using IniParser;
using IniParser.Model;
using System.Diagnostics;

namespace CustomLanternPatcher
{
    internal class Settings
    {
        public string fxr;
        public string color_xpath;
        public string sp_color_xpath;
        public string position_xpath;
        public string radius_xpath;
        public string intensity_xpath;

        public Settings(string file)
        {
            fxr = @"sfx\effect\f000302421.fxr";
            color_xpath = @"//FFXDrawEntityHost[@Unk00='609']/Properties1/FFXProperty[1][@Unk00='35']/Section11s";
            sp_color_xpath = @"//FFXDrawEntityHost[@Unk00='609']/Properties1/FFXProperty[2][@Unk00='35']/Section11s";
            position_xpath = @"//FFXDrawEntityHost[@Unk00='609']/../FFXDrawEntityHost[@Unk00='35']/Section11s1";
            radius_xpath = @"//FFXDrawEntityHost[@Unk00='609']/Properties1/FFXProperty[@Unk00='32'][1]/Section11s";
            intensity_xpath = @"//FFXDrawEntityHost[@Unk00='609']/Properties2/FFXProperty[@Unk00='32']/Section11s";

            Load(file);
        }

        public void Load(string file)
        {
            IniData data = new();

            // default values
            var config = data["config"];
            config["fxr"] = fxr;
            config["color_xpath"] = color_xpath;
            config["sp_color_xpath"] = sp_color_xpath;
            config["position_xpath"] = position_xpath;
            config["radius_xpath"] = radius_xpath;
            config["intensity_xpath"] = intensity_xpath;

            var parser = new FileIniDataParser();

            // merge default values with user settings
            // user settings overwrite default values
            if (File.Exists(file))
            {
                var userData = parser.ReadFile(file);
                data.Merge(userData);
            }
            else
            {
                Console.WriteLine($"Configuration file \"{file}\" does not exist. Default values will be used.");
            }

            // write ini
            parser.WriteFile(file, data);

            // get values
            config = data["config"];
            fxr = config["fxr"];
            color_xpath = config["color_xpath"];
            sp_color_xpath = config["sp_color_xpath"];
            position_xpath = config["position_xpath"];
            radius_xpath = config["radius_xpath"];
            intensity_xpath = config["intensity_xpath"];
        }
    }

}
