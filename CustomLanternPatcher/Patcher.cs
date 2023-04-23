using SoulsFormats;
using System.Diagnostics;
using System.Xml.Serialization;
using System.Xml;
using IniParser;
using System.Drawing;

namespace CustomLanternPatcher
{
    internal static class Patcher
    {
        private static void WriteXMLValue(float? f, XmlNode node, int index)
        {
            if (f != null)
            {
                XmlNode? intNode = node.SelectSingleNode($"int[{index}]");
                if (intNode == null)
                {
                    Console.Error.WriteLine($"XML node with index={index} not found." +
                        $"{Environment.NewLine}" +
                        $"Parent node:" +
                        $"{Environment.NewLine}```" +
                        node.OuterXml.ToString() +
                        $"```{Environment.NewLine}");

                    return;
                }

                if (intNode.FirstChild != null)
                {
                    intNode.FirstChild.Value = BitConverter.SingleToInt32Bits((float)f).ToString();
                }
            }
        }

        public static void Patch(string sourceFile, string outFile, Settings cfg, LanternSettings lanternCfg)
        {
            string fxrFile = cfg.fxr;
            string fileName = Path.GetFileName(sourceFile);

            if (!DCX.Is(sourceFile))
            {
                Console.Error.WriteLine($"File format is not DCX: {fileName}");
                Environment.Exit(1);
            }

            Console.WriteLine($"Decompressing DCX: {fileName}");
            byte[] dcxBytes = DCX.Decompress(sourceFile, out DCX.Type compression);



            if (!BND4.Is(dcxBytes))
            {
                Environment.Exit(1);
            }

            Console.WriteLine($"Unpacking BND4: {fileName}");
            var bnd = new BND4Reader(dcxBytes)
            {
                Compression = compression
            };

            BinderFileHeader? fileHeader = null;
            int fxrFileIndex = 0;
            for (int i = 0; i < bnd.Files.Count; i++)
            {
                if (bnd.Files[i].Name.EndsWith(fxrFile))
                {
                    fileHeader = bnd.Files[i];
                    fxrFileIndex = i;
                    break;
                }
            }

            if (fileHeader == null)
            {
                Console.Error.WriteLine($"Cannot find FXR file: {fxrFile}");
                Environment.Exit(1);
            }

            byte[] fxrBytes = bnd.ReadFile(fileHeader);
            FXR3 fxr = FXR3.Read(fxrBytes);

            XmlSerializer xmlSerializer = new(fxr.GetType());
            StringWriter writer = new();
            xmlSerializer.Serialize(writer, fxr);

            XmlDocument doc = new();
            doc.LoadXml(writer.ToString());
            writer.Dispose();
            XmlNode? root = doc.DocumentElement;
            if (root == null)
            {
                Console.Error.WriteLine("XML conversion error. XML document seems empty.");
                Environment.Exit(1);
            }

            #region patch XML

            XmlNode? color = root.SelectSingleNode(cfg.color_xpath);
            if (color == null)
            {
                Console.Error.WriteLine("XML conversion error. Cannot find color node.");
            }
            else
            {
                WriteXMLValue(lanternCfg.red, color, 1);
                WriteXMLValue(lanternCfg.green, color, 2);
                WriteXMLValue(lanternCfg.blue, color, 3);
                WriteXMLValue(lanternCfg.alpha, color, 4);
            }

            //ActionID="35"> <!-- Controls the position and rotation of the effect -->
            XmlNode? sp_color = root.SelectSingleNode(cfg.sp_color_xpath);
            if (sp_color == null)
            {
                Console.Error.WriteLine("XML conversion error. Cannot find sp_color node.");
            }
            else
            {
                WriteXMLValue(lanternCfg.sp_red, sp_color, 1);
                WriteXMLValue(lanternCfg.sp_green, sp_color, 2);
                WriteXMLValue(lanternCfg.sp_blue, sp_color, 3);
                WriteXMLValue(lanternCfg.sp_alpha, sp_color, 4);
            }

            // position and rotation
            XmlNode? position = root.SelectSingleNode(cfg.position_xpath);
            if (position == null)
            {
                Console.Error.WriteLine("XML conversion error. Cannot find position node.");
            }
            else
            {
                WriteXMLValue(lanternCfg.x, position, 1);
                WriteXMLValue(lanternCfg.y, position, 2);
                WriteXMLValue(lanternCfg.z, position, 3);
            }

            // radius
            XmlNode? radius = root.SelectSingleNode(cfg.radius_xpath);
            if (radius == null)
            {
                Console.Error.WriteLine("XML conversion error. Cannot find radius node.");
            }
            else
            {
                WriteXMLValue(lanternCfg.radius, radius, 1);
            }

            // luminous intensity
            XmlNode? intensity = root.SelectSingleNode(cfg.intensity_xpath);
            if (intensity == null)
            {
                Console.Error.WriteLine("XML conversion error. Cannot find intensity node.");
            }
            else
            {
                WriteXMLValue(lanternCfg.intensity, intensity, 1);
            }

            #endregion

            XmlReader reader = new XmlNodeReader(doc);
            FXR3? fxrPatched = (FXR3?)xmlSerializer.Deserialize(reader);
            if (fxrPatched == null)
            {
                Console.Error.WriteLine("XML deserialization failed.");
                Environment.Exit(1);
            }

            if (!fxrPatched.Validate(out Exception ex))
            {
                throw ex;
            }

            // repack
            var newBnd = new BND4
            {
                Compression = bnd.Compression,
                Version = bnd.Version,
                Format = bnd.Format,
                BigEndian = bnd.BigEndian,
                BitBigEndian = bnd.BitBigEndian,
                Unicode = bnd.Unicode,
                Extended = bnd.Extended,
                Unk04 = bnd.Unk04,
                Unk05 = bnd.Unk05
            };

            Console.WriteLine("Read and add FXR files");
            for (int i = 0; i < bnd.Files.Count; i++)
            {
                var flags = bnd.Files[i].Flags;
                var id = bnd.Files[i].ID;
                var name = bnd.Files[i].Name;
                byte[] bytes;
                if (i != fxrFileIndex)
                {
                    bytes = bnd.ReadFile(bnd.Files[i]);
                }
                else
                {
                    bytes = fxrPatched.Write(fxr.Compression); // patched FXR version
                }
                newBnd.Files.Add(new BinderFile(flags, id, name, bytes));
            }

            Console.WriteLine($"Write file: {outFile}");
            Console.WriteLine("Wait... This may take several minutes.");
            newBnd.Write(outFile);
        }
    }
}
