using System;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Data;
using ColorPicker;
using System.Globalization;
using System.Collections.Generic;
using IniParser;
using System.Windows.Controls;
using System.Text.RegularExpressions;
using ProcessMemoryUtilities.Native;
using System.Windows.Input;
using System.Windows.Media;

namespace CustomLanternTester
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        #region API Windows

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern void GetSystemInfo(ref SYSTEM_INFO Info);

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern IntPtr OpenProcess(
            uint processAccess,
            bool bInheritHandle,
            uint processId);

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern bool ReadProcessMemory(
           IntPtr hProcess,
           IntPtr lpBaseAddress,
           byte[] lpBuffer,
           ulong nSize,
           out ulong lpNumberOfBytesRead);

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern int VirtualQueryEx(
            IntPtr hProcess,
            IntPtr lpAddress,
            out MEMORY_BASIC_INFORMATION lpBuffer,
            ulong dwLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern bool WriteProcessMemory(
            IntPtr hProcess,
            IntPtr lpBaseAddress,
            byte[] lpBuffer,
            int nSize,
            out ulong lpNumberOfBytesWritten);


        [StructLayout(LayoutKind.Sequential)]
        internal struct SYSTEM_INFO
        {
            internal ushort wProcessorArchitecture;
            internal ushort wReserved;
            internal uint dwPageSize;
            internal IntPtr lpMinimumApplicationAddress;
            internal IntPtr lpMaximumApplicationAddress;
            internal IntPtr dwActiveProcessorMask;
            internal uint dwNumberOfProcessors;
            internal uint dwProcessorType;
            internal uint dwAllocationGranularity;
            internal ushort wProcessorLevel;
            internal ushort wProcessorRevision;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct MEMORY_BASIC_INFORMATION
        {
            public ulong BaseAddress;
            public ulong AllocationBase;
            public int AllocationProtect;
            public int __alignment1;
            public ulong RegionSize;
            public int State;
            public int Protect;
            public int Type;
            public int __alignment2;
        }

        #endregion

        const uint PROCESS_VM_READ = 0x0010;
        const uint PROCESS_VM_WRITE = 0x0020;
        const uint PROCESS_QUERY_INFORMATION = 0x0400;

        const uint PAGE_READWRITE = 0x00000004;
        const uint MEM_COMMIT = 0x1000;
        const uint MEM_PRIVATE = 0x20000;

        const ulong REGION_SIZE = 0xA0000000;

        private static IntPtr handle = IntPtr.Zero;
        private static IntPtr baseAddr = IntPtr.Zero;

        public struct Light // comments = default values
        {
            public float red;   // 1.0
            public float green; // 0.6313726
            public float blue;  // 0.36862746
            public float alpha; // 1.0

            public float sp_red;   // 1.0
            public float sp_green; // 0.6313726
            public float sp_blue;  // 0.36862746
            public float sp_alpha; // 1.0

            public float radius; // 16.0

            public float intensity; // 1.25

            public float x; // 0.0
            public float y; // 0.0
            public float z; // 0.0
        }

        public struct Offset
        {
            public const int red = 0x1CDC;
            public const int green = 0x1CE0;
            public const int blue = 0x1CE4;
            public const int alpha = 0x1CE8;

            public const int sp_red = 0x1CEC;
            public const int sp_green = 0x1CF0;
            public const int sp_blue = 0x1CF4;
            public const int sp_alpha = 0x1CF8;

            public const int radius = 0x1CFC;
            public const int intensity = 0x1D0C;

            public const int x = 0x1C1C;
            public const int y = 0x1C20;
            public const int z = 0x1C24;
        }

        private static IntPtr GetFXRBaseAddress()
        {
            // Get the process with the specified name
            Process? process = Process.GetProcessesByName("eldenring").FirstOrDefault();
            if (process == null)
            {
                return IntPtr.Zero;
            }

            int pid = process.Id;
            process.Dispose();

            handle = OpenProcess(
                PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION,
                false,
                (uint)pid);

            SYSTEM_INFO sysinfo = new();
            GetSystemInfo(ref sysinfo);

            MEMORY_BASIC_INFORMATION mbi = new();
            ulong mbiSize = (ulong)Marshal.SizeOf(mbi);
            ulong startAddress = 0;
            ulong currentRegionAddress = startAddress;
            byte[] buffer = new byte[sysinfo.dwPageSize];

            while (true)
            {
                if (VirtualQueryEx(handle, (IntPtr)currentRegionAddress,
                    out mbi, mbiSize) == 0)
                {
                    int lastError = Marshal.GetLastWin32Error();
                    CloseHandle(handle);
                    MessageBox.Show("VirtualQueryEx() failed with error " + lastError);
                    return IntPtr.Zero;
                }

                if (mbi.RegionSize == REGION_SIZE &&
                    mbi.Protect == PAGE_READWRITE &&
                    mbi.State == MEM_COMMIT &&
                    mbi.Type == MEM_PRIVATE)
                {
                    ulong currentPageAddress = mbi.BaseAddress;
                    ulong endPageAddress = currentPageAddress + mbi.RegionSize;

                    // browse memory pages
                    while (currentPageAddress < endPageAddress)
                    {
                        // read a memory page
                        if (!ReadProcessMemory(handle, (IntPtr)currentPageAddress,
                            buffer, sysinfo.dwPageSize,
                            out ulong _))
                        {
                            int lastError = Marshal.GetLastWin32Error();
                            CloseHandle(handle);
                            MessageBox.Show("ReadProcessMemory() failed with error " + lastError);
                            return IntPtr.Zero;
                        }

                        int offset = 0;

                        // find the signature in the readed page memory
                        while (offset < sysinfo.dwPageSize)
                        {
                            if (BitConverter.ToUInt64(buffer, offset) == 0x0005000000525846)
                            {
                                if (BitConverter.ToUInt64(buffer, offset + 8) == 0x00049D5500000001)
                                {
                                    return (IntPtr)(currentPageAddress + (ulong)offset);
                                }
                            }
                            // next byte
                            offset += 8;
                        }
                        // next page
                        currentPageAddress += sysinfo.dwPageSize;
                    }
                }
                // next region
                currentRegionAddress += mbi.RegionSize;
            }
        }

        static bool WriteTextBoxValueToMemory(TextBox textBox, int offset, out float? color)
        {
            string filter = "[^\\d\\.]";
            string text = textBox.Text;
            text = Regex.Replace(text, filter, "");
            textBox.Text = text;
            if (text.Length > 0)
            {
                color = Convert.ToSingle(text);
                byte[] colorBytes = BitConverter.GetBytes((float)color);

                // copy colors to the buffer
                int size = Marshal.SizeOf(color);
                IntPtr buffer = Marshal.AllocHGlobal(size);
                Marshal.Copy(colorBytes, 0, buffer, colorBytes.Length);

                unsafe
                {
                    // fast `write process memory`
                    NtDll.NtWriteVirtualMemory(handle,
                        IntPtr.Add(baseAddr, offset),
                        buffer, (IntPtr)size, out IntPtr _);
                }

                return true;
            }

            color = null;
            return false;
        }

        public static bool ReadLightValuesInGameMemory(ref Light light)
        {
            ulong size = 8 * sizeof(float); // 4 colors + 4 sp_colors
            byte[] buffer = new byte[size];

            // read colors and specular colors
            if (!ReadProcessMemory(handle, IntPtr.Add(baseAddr, Offset.red), buffer, size, out ulong _))
            {
                MessageBox.Show("ReadProcessMemory() failed with error " + Marshal.GetLastWin32Error());
                return false;
            }

            // colors
            light.red = BitConverter.ToSingle(buffer, 0);       // 1.0
            light.green = BitConverter.ToSingle(buffer, 4);     // 0.6313726
            light.blue = BitConverter.ToSingle(buffer, 8);      // 0.36862746
            light.alpha = BitConverter.ToSingle(buffer, 12);    // 1.0

            // specular colors
            light.sp_red = BitConverter.ToSingle(buffer, 16);   // 1.0
            light.sp_green = BitConverter.ToSingle(buffer, 20); // 0.6313726
            light.sp_blue = BitConverter.ToSingle(buffer, 24);  // 0.36862746
            light.sp_alpha = BitConverter.ToSingle(buffer, 28); // 1.0

            // radius
            if (!ReadProcessMemory(handle, IntPtr.Add(baseAddr, Offset.radius), buffer, sizeof(float), out ulong _))
            {
                MessageBox.Show("ReadProcessMemory() failed with error " + Marshal.GetLastWin32Error());
                return false;
            }
            light.radius = BitConverter.ToSingle(buffer, 0); // 16.0

            // luminous intensity
            if (!ReadProcessMemory(handle, IntPtr.Add(baseAddr, Offset.intensity), buffer, sizeof(float), out ulong _))
            {
                MessageBox.Show("ReadProcessMemory() failed with error " + Marshal.GetLastWin32Error());
                return false;
            }
            light.intensity = BitConverter.ToSingle(buffer, 0); // 1.25


            size = 3 * sizeof(float);
            buffer = new byte[size];

            // read position
            if (!ReadProcessMemory(handle, IntPtr.Add(baseAddr, Offset.x), buffer, size, out ulong _))
            {
                MessageBox.Show("ReadProcessMemory() failed with error " + Marshal.GetLastWin32Error());
                return false;
            }

            light.x = BitConverter.ToSingle(buffer, 0); // 0.0
            light.y = BitConverter.ToSingle(buffer, 4); // 0.0
            light.z = BitConverter.ToSingle(buffer, 8); // 0.0

            return true;
        }

        private static bool TryParsePositionSettingKey(string pos, ref string? x, ref string? y, ref string? z)
        {
            /*
             * Syntax:
             * pos = <x>, <y>, <z>
             */
            RegexOptions options = RegexOptions.IgnoreCase | RegexOptions.Singleline;
            string pattern = @"^([\d\.]+)\s*,\s*([\d\.]+)\s*,\s*([\d\.]+)$";
            Match m = Regex.Match(pos, pattern, options);
            if (m.Success)
            {
                x = m.Groups[1].Value;
                y = m.Groups[2].Value;
                z = m.Groups[3].Value;

                return true;
            }

            return false;
        }

        private static bool TryParseColorSettingKey(string color,
            ref string? red, ref string? green, ref string? blue, ref string? alpha)
        {
            /*
             * Syntax:
             * color = { [#|0x]<[AA]RRGGBB> | <red>, <green>, <blue>[, <alpha>] }
            */
            RegexOptions options = RegexOptions.IgnoreCase | RegexOptions.Singleline;

            // [#|0x]<[AA]RRGGBB>
            string pattern = @"^(?:#|0x)?([a-f0-9]{2})?([a-f0-9]{2})([a-f0-9]{2})([a-f0-9]{2})$";
            Match m = Regex.Match(color, pattern, options);
            if (m.Success)
            {
                alpha = m.Groups[1].Success ? m.Groups[1].Value : "FF";
                red = m.Groups[2].Value;
                green = m.Groups[3].Value;
                blue = m.Groups[4].Value;

                // convert string hex values to float to string => #AARRGGBB
                Color mediaColor = (Color)ColorConverter.ConvertFromString("#" + alpha + red + green + blue);
                red = (mediaColor.R / 255F).ToString();
                green = (mediaColor.G / 255F).ToString();
                blue = (mediaColor.B / 255F).ToString();
                alpha = (mediaColor.A / 255F).ToString();

                return true;
            }

            // <red>, <green>, <blue>, <alpha>
            pattern = @"^([\d\.]+)\s*,\s*([\d\.]+)\s*,\s*([\d\.]+)\s*,\s*([\d\.]+)$";
            m = Regex.Match(color, pattern, options);
            if (m.Success)
            {
                red = m.Groups[1].Value;
                green = m.Groups[2].Value;
                blue = m.Groups[3].Value;
                alpha = m.Groups[4].Value;

                return true;
            }

            return false;
        }

        private void LoadSettings()
        {
            List<LightSource> LightSourceList = new();

            var parser = new FileIniDataParser();

            IniData data = parser.ReadFile("custom-lantern-tester.ini");

            // iterate through all the sections
            foreach (var section in data.Sections)
            {
                string? color, sp_color,
                    red = null, green = null, blue = null, alpha = null,
                    sp_red = null, sp_green = null, sp_blue = null, sp_alpha = null;

                // if key `color` or `sp_color` exists,
                // so keys `red` to `alpha` or `sp_red` to `sp_alpha` are ignored

                color = section.Properties.FindByKey("color")?.Value;
                if (color != null)
                {
                    // case:
                    // color=[HEX_COLOR | float list]
                    if (!TryParseColorSettingKey(color, ref red, ref green, ref blue, ref alpha))
                    {
                        MessageBox.Show(@"Unable to parse `color` " + color);
                    }
                }
                else
                {
                    // case:
                    // red=float
                    // blue=float
                    // ...
                    // alpha=float
                    red = section.Properties.FindByKey("red")?.Value;
                    green = section.Properties.FindByKey("green")?.Value;
                    blue = section.Properties.FindByKey("blue")?.Value;
                    alpha = section.Properties.FindByKey("alpha")?.Value;
                }

                sp_color = section.Properties.FindByKey("sp_color")?.Value;
                if (sp_color != null)
                {
                    // case:
                    // sp_color=[HEX_COLOR | float list]
                    if (!TryParseColorSettingKey(sp_color, ref sp_red, ref sp_green, ref sp_blue, ref sp_alpha))
                    {
                        MessageBox.Show(@"Unable to parse `sp_color` " + sp_color);
                    }
                }
                else
                {
                    // case:
                    // sp_red=float
                    // sp_blue=float
                    // ...
                    // sp_alpha=float
                    sp_red = section.Properties.FindByKey("sp_red")?.Value;
                    sp_green = section.Properties.FindByKey("sp_green")?.Value;
                    sp_blue = section.Properties.FindByKey("sp_blue")?.Value;
                    sp_alpha = section.Properties.FindByKey("sp_alpha")?.Value;
                }

                string? x = null, y = null, z = null;

                // XYZ position
                string? pos = section.Properties.FindByKey("pos")?.Value;
                if (pos != null)
                {
                    // case:
                    // pos=<x>, <y>, <z>
                    if (!TryParsePositionSettingKey(pos, ref x, ref y, ref z))
                    {
                        MessageBox.Show(@"Unable to parse `pos` " + pos);
                    }
                }
                else
                {
                    x = section.Properties.FindByKey("x")?.Value;
                    y = section.Properties.FindByKey("y")?.Value;
                    z = section.Properties.FindByKey("z")?.Value;
                }

                string? radius = section.Properties.FindByKey("radius")?.Value;
                string? intensity = section.Properties.FindByKey("intensity")?.Value;

                string? comment = section.Properties.FindByKey("comment")?.Value;

                LightSource lightSource = new(
                    section.Name,
                    red, green, blue, alpha,
                    sp_red, sp_green, sp_blue, sp_alpha,
                    x, y, z,
                    radius, intensity,
                    comment);

                LightSourceList.Add(lightSource);
            }

            SavedLightSources.ItemsSource = LightSourceList;
        }

        public MainWindow()
        {
            InitializeComponent();

            EventManager.RegisterClassHandler(typeof(TextBox), TextBox.GotFocusEvent,
                new RoutedEventHandler(TextBox_GotFocus));

            CultureInfo.CurrentCulture = CultureInfo.InvariantCulture;

            Colorpicker.ColorChanged += Colorpicker_ColorChanged;
            SpColorpicker.ColorChanged += SpColorpicker_ColorChanged;

            // find FXR base address
            baseAddr = GetFXRBaseAddress();
            if (baseAddr != IntPtr.Zero)
            {
                TextBoxBaseAddr.Text = baseAddr.ToString("X");
                TextBoxBaseAddr.ClearValue(ForegroundProperty);
            }

            // load settings (saved light sources) from `custom-lantern-tester.ini`
            LoadSettings();
        }

        #region GUI events

        private void Colorpicker_ColorChanged(object sender, RoutedEventArgs e)
        {
            float[] colors = {
                (float)(Colorpicker.Color.RGB_R / 255),
                (float)(Colorpicker.Color.RGB_G / 255),
                (float)(Colorpicker.Color.RGB_B / 255),
                (float)(Colorpicker.Color.A / 255)
            };

            // copy colors to the buffer
            int size = Marshal.SizeOf(colors[0]) * colors.Length;
            IntPtr buffer = Marshal.AllocHGlobal(size);
            Marshal.Copy(colors, 0, buffer, colors.Length);

            unsafe
            {
                // fast `write process memory`
                NtDll.NtWriteVirtualMemory(handle,
                    IntPtr.Add(baseAddr, Offset.red),
                    buffer, (IntPtr)size, out IntPtr _);
            }

            // fill textboxes with the color changed
            TextBoxR.Text = colors[0].ToString();
            TextBoxG.Text = colors[1].ToString();
            TextBoxB.Text = colors[2].ToString();
            TextBoxA.Text = colors[3].ToString();

            // free the unmanaged memory
            Marshal.FreeHGlobal(buffer);
        }

        private void SpColorpicker_ColorChanged(object sender, RoutedEventArgs e)
        {
            float[] colors = {
                (float)SpColorpicker.Color.RGB_R / 255,
                (float)SpColorpicker.Color.RGB_G / 255,
                (float)SpColorpicker.Color.RGB_B / 255,
                (float)SpColorpicker.Color.A / 255,
            };

            // copy colors to the buffer
            int size = Marshal.SizeOf(colors[0]) * colors.Length;
            IntPtr buffer = Marshal.AllocHGlobal(size);
            Marshal.Copy(colors, 0, buffer, colors.Length);

            unsafe
            {
                NtDll.NtWriteVirtualMemory(handle,
                    IntPtr.Add(baseAddr, Offset.sp_red),
                    buffer, (IntPtr)size, out IntPtr _);
            }

            // fill textboxes with the color changed
            TextBoxSpR.Text = colors[0].ToString();
            TextBoxSpG.Text = colors[1].ToString();
            TextBoxSpB.Text = colors[2].ToString();
            TextBoxSpA.Text = colors[3].ToString();

            // free the unmanaged memory
            Marshal.FreeHGlobal(buffer);
        }

        private void ButtonBaseAddr_Click(object sender, RoutedEventArgs e)
        {
            Colorpicker.ColorChanged -= Colorpicker_ColorChanged;
            SpColorpicker.ColorChanged -= SpColorpicker_ColorChanged;

            baseAddr = GetFXRBaseAddress();
            if (baseAddr != IntPtr.Zero)
            {
                TextBoxBaseAddr.Text = baseAddr.ToString("X");
                TextBoxBaseAddr.ClearValue(ForegroundProperty);
                Colorpicker.ColorChanged += Colorpicker_ColorChanged;
                SpColorpicker.ColorChanged += SpColorpicker_ColorChanged;
            }
        }

        private void CheckBoxSync_Checked(object sender, RoutedEventArgs e)
        {
            Binding binding = new()
            {
                Source = Colorpicker,
                Path = new PropertyPath("ColorState"),
                Mode = BindingMode.TwoWay
            };
            BindingOperations.SetBinding(SpColorpicker,
                PickerControlBase.ColorStateProperty,
                binding);
        }

        private void CheckBoxSync_Unchecked(object sender, RoutedEventArgs e)
        {
            SpColorpicker.ColorChanged -= SpColorpicker_ColorChanged;
            BindingOperations.ClearBinding(SpColorpicker, PickerControlBase.ColorStateProperty);

            // SpColorpicker.ColorState is reseted, we need to reload values from Colorpicker.ColorState
            SpColorpicker.ColorState = Colorpicker.ColorState;

            SpColorpicker.ColorChanged += SpColorpicker_ColorChanged;
        }

        private void ButtonCopyToClipboard_Click(object sender, RoutedEventArgs e)
        {
            Light light = new();
            if (!ReadLightValuesInGameMemory(ref light))
            {
                return;
            }

            // INI content to paste in `custom-lantern.ini`
            string ini = $@"[config]
red={TextBoxR.Text}
green={TextBoxG.Text}
blue={TextBoxB.Text}
alpha={TextBoxA.Text}
sp_red={TextBoxSpR.Text}
sp_green={TextBoxSpG.Text}
sp_blue={TextBoxSpB.Text}
sp_alpha={TextBoxSpA.Text}
radius={TextBoxRadius.Text}
intensity={TextBoxIntensity.Text}
x={TextBoxX.Text}
y={TextBoxY.Text}
z={TextBoxZ.Text}";

            Clipboard.SetText(ini);
        }

        private void SavedLightSources_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // remove synchronization between color pickers before loading values
            CheckBoxSync.IsChecked = false;

            if (SavedLightSources.SelectedItem is not LightSource lightSource) { return; }
            LoadLightSource(ref lightSource);

            WriteAllValuesToMemory();
        }

        private void LoadLightSource(ref LightSource lightSource)
        {
            // colors
            if (FilterR.IsChecked == true) { TextBoxR.Text = lightSource.Red; }
            if (FilterG.IsChecked == true) { TextBoxG.Text = lightSource.Green; }
            if (FilterB.IsChecked == true) { TextBoxB.Text = lightSource.Blue; }
            if (FilterA.IsChecked == true) { TextBoxA.Text = lightSource.Alpha; }

            // specular colors
            if (FilterSpR.IsChecked == true) { TextBoxSpR.Text = lightSource.SpRed; }
            if (FilterSpG.IsChecked == true) { TextBoxSpG.Text = lightSource.SpGreen; }
            if (FilterSpB.IsChecked == true) { TextBoxSpB.Text = lightSource.SpBlue; }
            if (FilterSpA.IsChecked == true) { TextBoxSpA.Text = lightSource.SpAlpha; }

            // position
            if (FilterX.IsChecked == true) { TextBoxX.Text = lightSource.X; }
            if (FilterY.IsChecked == true) { TextBoxY.Text = lightSource.Y; }
            if (FilterZ.IsChecked == true) { TextBoxZ.Text = lightSource.Z; }

            if (FilterRadius.IsChecked == true) { TextBoxRadius.Text = lightSource.Radius; }
            if (FilterIntensity.IsChecked == true) { TextBoxIntensity.Text = lightSource.Intensity; }
        }

        private void ButtonRefresh_Click(object sender, RoutedEventArgs e)
        {
            // read all values in game process memory
            Light light = new();
            if (!ReadLightValuesInGameMemory(ref light))
            {
                return;
            }

            // fill textboxes
            //
            // RGBA
            TextBoxR.Text = light.red.ToString();
            TextBoxG.Text = light.green.ToString();
            TextBoxB.Text = light.blue.ToString();
            TextBoxA.Text = light.alpha.ToString();
            //
            // specular RGBA
            TextBoxSpR.Text = light.sp_red.ToString();
            TextBoxSpG.Text = light.sp_green.ToString();
            TextBoxSpB.Text = light.sp_blue.ToString();
            TextBoxSpA.Text = light.sp_alpha.ToString();
            //
            // XYZ
            TextBoxX.Text = light.x.ToString();
            TextBoxY.Text = light.y.ToString();
            TextBoxZ.Text = light.z.ToString();
            //
            TextBoxRadius.Text = light.radius.ToString();
            TextBoxIntensity.Text = light.intensity.ToString();

            // remove events before loading values
            Colorpicker.ColorChanged -= Colorpicker_ColorChanged;
            SpColorpicker.ColorChanged -= SpColorpicker_ColorChanged;

            // remove synchronization between color pickers before loading values
            CheckBoxSync.IsChecked = false;

            // apply color read from memory to colors pickers
            //
            // RGBA
            Colorpicker.Color.RGB_R = light.red * 255;
            Colorpicker.Color.RGB_G = light.green * 255;
            Colorpicker.Color.RGB_B = light.blue * 255;
            Colorpicker.Color.A = light.alpha * 255;
            //
            // specular RGBA
            SpColorpicker.Color.RGB_R = light.sp_red * 255;
            SpColorpicker.Color.RGB_G = light.sp_green * 255;
            SpColorpicker.Color.RGB_B = light.sp_blue * 255;
            SpColorpicker.Color.A = light.sp_alpha * 255;

            // restore events
            Colorpicker.ColorChanged += Colorpicker_ColorChanged;
            SpColorpicker.ColorChanged += SpColorpicker_ColorChanged;
        }

        private void TextBox_GotFocus(object sender, RoutedEventArgs e)
        {
            TextBox textBox = (TextBox)sender;
            textBox.SelectAll();
        }

        private void TextBoxSetKelvin_LostFocus(object sender, RoutedEventArgs e)
        {
            string text = System.Text.RegularExpressions.Regex.Replace(TextBoxSetKelvin.Text, "\\D", "");
            TextBoxSetKelvin.Text = text;
            if (text.Length < 1) { return; }

            double kelvinT = Convert.ToDouble(text);

            if (kelvinT < 1000) { TextBoxSetKelvin.Text = "1000"; }
            if (kelvinT > 40000) { TextBoxSetKelvin.Text = "40000"; }
        }

        private void SliderKelvin_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            int[] rgb = LightsUtils.GetRgbFromTemperature(SliderKelvin.Value);
            Colorpicker.Color.RGB_R = rgb[0];
            Colorpicker.Color.RGB_G = rgb[1];
            Colorpicker.Color.RGB_B = rgb[2];
        }

        // Apply color temperature
        private void ButtonApplySetKelvin_Click(object sender, RoutedEventArgs e)
        {
            string text = Regex.Replace(TextBoxSetKelvin.Text, "\\D", "");
            TextBoxSetKelvin.Text = text;
            if (text.Length < 1) { return; }

            double kelvinT = Convert.ToDouble(text);

            if (kelvinT < 1000)
            {
                kelvinT = 1000;
            }
            if (kelvinT > 40000)
            {
                kelvinT = 40000;
            }

            int[] rgb = LightsUtils.GetRgbFromTemperature(kelvinT);
            Colorpicker.Color.RGB_R = rgb[0];
            Colorpicker.Color.RGB_G = rgb[1];
            Colorpicker.Color.RGB_B = rgb[2];
        }

        #region TextBox `Light values`

        #region TextBoxRGBA

        private void TextBoxR_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                WriteTextBoxValueToMemory((TextBox)sender, Offset.red, out float? r);
                if (r != null) { Colorpicker.Color.RGB_R = (double)r * 255; }
            }
        }

        private void TextBoxG_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                WriteTextBoxValueToMemory((TextBox)sender, Offset.green, out float? g);
                if (g != null) { Colorpicker.Color.RGB_G = (double)g * 255; }
            }
        }

        private void TextBoxB_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                WriteTextBoxValueToMemory((TextBox)sender, Offset.blue, out float? b);
                if (b != null) { Colorpicker.Color.RGB_B = (double)b * 255; }
            }
        }

        private void TextBoxA_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                WriteTextBoxValueToMemory((TextBox)sender, Offset.alpha, out float? a);
                if (a != null) { Colorpicker.Color.A = (double)a * 255; }
            }
        }

        #endregion

        #region TextBoxSpRGBA

        private void TextBoxSpR_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                WriteTextBoxValueToMemory((TextBox)sender, Offset.sp_red, out float? spR);
                if (spR != null) { SpColorpicker.Color.RGB_R = (double)spR * 255; }
            }
        }

        private void TextBoxSpG_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                WriteTextBoxValueToMemory((TextBox)sender, Offset.sp_green, out float? spG);
                if (spG != null) { SpColorpicker.Color.RGB_G = (double)spG * 255; }
            }
        }

        private void TextBoxSpB_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                WriteTextBoxValueToMemory((TextBox)sender, Offset.sp_blue, out float? spB);
                if (spB != null) { SpColorpicker.Color.RGB_B = (double)spB * 255; }
            }
        }

        private void TextBoxSpA_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                WriteTextBoxValueToMemory((TextBox)sender, Offset.sp_alpha, out float? spA);
                if (spA != null) { SpColorpicker.Color.A = (double)spA * 255; }
            }
        }

        #endregion

        #region TextBoxXYZ

        private void TextBoxX_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return) { WriteTextBoxValueToMemory((TextBox)sender, Offset.x, out float? _); }
        }

        private void TextBoxY_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return) { WriteTextBoxValueToMemory((TextBox)sender, Offset.y, out float? _); }
        }

        private void TextBoxZ_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return) { WriteTextBoxValueToMemory((TextBox)sender, Offset.z, out float? _); }
        }

        #endregion

        private void TextBoxRadius_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return) { WriteTextBoxValueToMemory((TextBox)sender, Offset.radius, out float? _); }
        }

        private void TextBoxIntensity_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return) { WriteTextBoxValueToMemory((TextBox)sender, Offset.intensity, out float? _); }
        }

        #endregion

        private void ButtonApplyWriteMemory_Click(object sender, RoutedEventArgs e)
        {
            WriteAllValuesToMemory();
        }

        private void WriteAllValuesToMemory()
        {
            Colorpicker.ColorChanged -= Colorpicker_ColorChanged;
            SpColorpicker.ColorChanged -= SpColorpicker_ColorChanged;

            // RGBA
            WriteTextBoxValueToMemory(TextBoxR, Offset.red, out float? r);
            WriteTextBoxValueToMemory(TextBoxG, Offset.green, out float? g);
            WriteTextBoxValueToMemory(TextBoxB, Offset.blue, out float? b);
            WriteTextBoxValueToMemory(TextBoxA, Offset.alpha, out float? a);
            if (r != null) { Colorpicker.Color.RGB_R = (double)r * 255; }
            if (g != null) { Colorpicker.Color.RGB_G = (double)g * 255; }
            if (b != null) { Colorpicker.Color.RGB_B = (double)b * 255; }
            if (a != null) { Colorpicker.Color.A = (double)a * 255; }

            // specular RGBA
            WriteTextBoxValueToMemory(TextBoxSpR, Offset.sp_red, out float? spR);
            WriteTextBoxValueToMemory(TextBoxSpG, Offset.sp_green, out float? spG);
            WriteTextBoxValueToMemory(TextBoxSpB, Offset.sp_blue, out float? spB);
            WriteTextBoxValueToMemory(TextBoxSpA, Offset.sp_alpha, out float? spA);
            if (spR != null) { SpColorpicker.Color.RGB_R = (double)spR * 255; }
            if (spG != null) { SpColorpicker.Color.RGB_G = (double)spG * 255; }
            if (spB != null) { SpColorpicker.Color.RGB_B = (double)spB * 255; }
            if (spA != null) { SpColorpicker.Color.A = (double)spA * 255; }

            // position
            WriteTextBoxValueToMemory(TextBoxX, Offset.x, out float? _);
            WriteTextBoxValueToMemory(TextBoxY, Offset.y, out float? _);
            WriteTextBoxValueToMemory(TextBoxZ, Offset.z, out float? _);

            WriteTextBoxValueToMemory(TextBoxRadius, Offset.radius, out float? _);
            WriteTextBoxValueToMemory(TextBoxIntensity, Offset.intensity, out float? _);

            Colorpicker.ColorChanged += Colorpicker_ColorChanged;
            SpColorpicker.ColorChanged += SpColorpicker_ColorChanged;
        }

        #endregion

        private void TextBoxSetKelvin_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                string text = Regex.Replace(TextBoxSetKelvin.Text, "\\D", "");
                TextBoxSetKelvin.Text = text;
                if (text.Length < 1) { return; }

                double kelvinT = Convert.ToDouble(text);

                if (kelvinT < 1000)
                {
                    kelvinT = 1000;
                    TextBoxSetKelvin.Text = "1000";
                }
                if (kelvinT > 40000)
                {
                    kelvinT = 40000;
                    TextBoxSetKelvin.Text = "40000";
                }

                int[] rgb = LightsUtils.GetRgbFromTemperature(kelvinT);
                Colorpicker.Color.RGB_R = rgb[0];
                Colorpicker.Color.RGB_G = rgb[1];
                Colorpicker.Color.RGB_B = rgb[2];
            }
        }

        private void ButtonLoadLightSource_Click(object sender, RoutedEventArgs e)
        {
            // remove synchronization between color pickers before loading values
            CheckBoxSync.IsChecked = false;

            if (SavedLightSources.SelectedItem is not LightSource lightSource) { return; }

            LoadLightSource(ref lightSource);

            WriteAllValuesToMemory();
        }

        private void ButtonApplySetKelvinFromSlider_Click(object sender, RoutedEventArgs e)
        {
            int[] rgb = LightsUtils.GetRgbFromTemperature(SliderKelvin.Value);
            Colorpicker.Color.RGB_R = rgb[0];
            Colorpicker.Color.RGB_G = rgb[1];
            Colorpicker.Color.RGB_B = rgb[2];
        }
    }
}
