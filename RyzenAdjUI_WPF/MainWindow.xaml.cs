using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using Microsoft.Win32;

namespace RyzenAdjUI_WPF {
    internal enum AccentState {
        ACCENT_DISABLED = 1,
        ACCENT_ENABLE_GRADIENT = 0,
        ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
        ACCENT_ENABLE_BLURBEHIND = 3,
        ACCENT_INVALID_STATE = 4
    }

    [StructLayout (LayoutKind.Sequential)]
    internal struct AccentPolicy {
        public AccentState AccentState;
        public int AccentFlags;
        public int GradientColor;
        public int AnimationId;
    }

    [StructLayout (LayoutKind.Sequential)]
    internal struct WindowCompositionAttributeData {
        public WindowCompositionAttribute Attribute;
        public IntPtr Data;
        public int SizeOfData;
    }

    internal enum WindowCompositionAttribute {
        // ...
        WCA_ACCENT_POLICY = 19
        // ...
    }
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window {
        private static string dir = System.IO.Path.GetDirectoryName (Process.GetCurrentProcess ().MainModule.FileName);
        private static string settingsFileName = "settings.bat";
        private string batFilePath = $"{dir}\\{settingsFileName}";

        [DllImport ("user32.dll")]
        internal static extern int SetWindowCompositionAttribute (IntPtr hwnd, ref WindowCompositionAttributeData data);

        public MainWindow () {
            InitializeComponent ();
        }

        private void Window_Loaded (object sender, RoutedEventArgs e) {
            EnableBlur ();
            loadSettings();
        }

        internal void EnableBlur () {
            var windowHelper = new WindowInteropHelper (this);

            var accent = new AccentPolicy ();
            accent.AccentState = AccentState.ACCENT_ENABLE_BLURBEHIND;

            var accentStructSize = Marshal.SizeOf (accent);

            var accentPtr = Marshal.AllocHGlobal (accentStructSize);
            Marshal.StructureToPtr (accent, accentPtr, false);

            var data = new WindowCompositionAttributeData ();
            data.Attribute = WindowCompositionAttribute.WCA_ACCENT_POLICY;
            data.SizeOfData = accentStructSize;
            data.Data = accentPtr;

            SetWindowCompositionAttribute (windowHelper.Handle, ref data);

            Marshal.FreeHGlobal (accentPtr);
        }

        private void Window_MouseDown (object sender, System.Windows.Input.MouseButtonEventArgs e) {
            DragMove ();
        }

        public void RcheckBox1_MouseDown (object sender, MouseButtonEventArgs e) {
            if (rcheckBox1.Fill != Brushes.Green) {
                rcheckBox1.Fill = Brushes.Green;
                cube1.Visibility = Visibility.Visible;
                rect1s.Visibility = Visibility.Visible;
                slider1.Visibility = Visibility.Visible;
                lab1.Visibility = Visibility.Visible;
            } else {
                rcheckBox1.Fill = Brushes.Transparent;
                cube1.Visibility = Visibility.Hidden;
                rect1s.Visibility = Visibility.Hidden;
                slider1.Visibility = Visibility.Hidden;
                lab1.Visibility = Visibility.Hidden;
            }

        }

        public void RcheckBox2_MouseDown (object sender, MouseButtonEventArgs e) {
            if (rcheckBox2.Fill != Brushes.Green) {
                rcheckBox2.Fill = Brushes.Green;
                cube2.Visibility = Visibility.Visible;
                rect2s.Visibility = Visibility.Visible;
                slider2.Visibility = Visibility.Visible;
                lab2.Visibility = Visibility.Visible;
            } else {
                rcheckBox2.Fill = Brushes.Transparent;
                cube2.Visibility = Visibility.Hidden;
                rect2s.Visibility = Visibility.Hidden;
                slider2.Visibility = Visibility.Hidden;
                lab2.Visibility = Visibility.Hidden;
            }
        }

        public void RcheckBox3_MouseDown (object sender, MouseButtonEventArgs e) {
            if (rcheckBox3.Fill != Brushes.Green) {
                rcheckBox3.Fill = Brushes.Green;
                cube3.Visibility = Visibility.Visible;
                rect3s.Visibility = Visibility.Visible;
                slider3.Visibility = Visibility.Visible;
                lab3.Visibility = Visibility.Visible;
            } else {
                rcheckBox3.Fill = Brushes.Transparent;
                cube3.Visibility = Visibility.Hidden;
                rect3s.Visibility = Visibility.Hidden;
                slider3.Visibility = Visibility.Hidden;
                lab3.Visibility = Visibility.Hidden;
            }
        }

        public void RcheckBox4_MouseDown (object sender, MouseButtonEventArgs e) {
            if (rcheckBox4.Fill != Brushes.Green) {
                rcheckBox4.Fill = Brushes.Green;
                cube4.Visibility = Visibility.Visible;
                rect4s.Visibility = Visibility.Visible;
                slider4.Visibility = Visibility.Visible;
                lab4.Visibility = Visibility.Visible;
            } else {
                rcheckBox4.Fill = Brushes.Transparent;
                cube4.Visibility = Visibility.Hidden;
                rect4s.Visibility = Visibility.Hidden;
                slider4.Visibility = Visibility.Hidden;
                lab4.Visibility = Visibility.Hidden;
            }
        }

        public void RcheckBox5_MouseDown (object sender, MouseButtonEventArgs e) {
            if (rcheckBox5.Fill != Brushes.Green) {
                rcheckBox5.Fill = Brushes.Green;
                cube5.Visibility = Visibility.Visible;
                rect5s.Visibility = Visibility.Visible;
                slider5.Visibility = Visibility.Visible;
                lab5.Visibility = Visibility.Visible;
            } else {
                rcheckBox5.Fill = Brushes.Transparent;
                cube5.Visibility = Visibility.Hidden;
                rect5s.Visibility = Visibility.Hidden;
                slider5.Visibility = Visibility.Hidden;
                lab5.Visibility = Visibility.Hidden;
            }
        }

        public void RcheckBox6_MouseDown (object sender, MouseButtonEventArgs e) {
            if (rcheckBox6.Fill != Brushes.Green) {
                rcheckBox6.Fill = Brushes.Green;
            } else {
                rcheckBox6.Fill = Brushes.Transparent;
            }
        }

        public void Button_Click (object sender, RoutedEventArgs e) {
            if (RunExe ()) {
                MessageBox.Show ("Settings successfully applied!");
            } else {
                MessageBox.Show ("ERROR: Failed to apply settings.");
            }
            // System.Windows.Application.Current.Shutdown();
        }

        // Returns true if successful, else returns false.
        public bool RunExe () {
            int spm = 15;
            int sfl = 30;
            int ssl = 25;
            int tmp = 80;
            int vrm = 30000;

            if (rcheckBox1.Fill == Brushes.Green)
                spm = int.Parse (slider1.Value.ToString ());
            if (rcheckBox2.Fill == Brushes.Green)
                sfl = int.Parse (slider2.Value.ToString ());
            if (rcheckBox3.Fill == Brushes.Green)
                ssl = int.Parse (slider3.Value.ToString ());
            if (rcheckBox4.Fill == Brushes.Green)
                tmp = int.Parse (slider4.Value.ToString ());
            if (rcheckBox5.Fill == Brushes.Green)
                vrm = int.Parse (slider5.Value.ToString ());

            string vrmhex = vrm.ToString ("X");

            string args = $"--stapm-limit={spm}000 --fast-limit={sfl}000 --slow-limit={ssl}000 --tctl-temp={tmp} --vrmmax-current=0x{vrmhex}";

            try {
                Process.Start (new ProcessStartInfo {
                    FileName = $"{dir}\\ryzenadj.exe",
                        UseShellExecute = false,
                        CreateNoWindow = true,
                        Arguments = args,
                        RedirectStandardOutput = true
                });
            } catch {
                MessageBox.Show ("Can't find ryzenadj.exe folder");
                return false;
            }
            CreateBat();
            if (rcheckBox6.Fill == Brushes.Green) {
                AutoStart ();
            } else {
                ClearAutoStart ();
            }
            return true;

        }

        public void AutoStart () {
            RegistryKey rkey = Registry.CurrentUser.OpenSubKey ("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", true);
            if (File.Exists(batFilePath))
            {
                rkey.SetValue("RyzenAdj", batFilePath);
            }
            else
            {
                MessageBox.Show("configuration file does not exist at path: " + batFilePath);
            }
        }

        public void ClearAutoStart () {
            RegistryKey rkey = Registry.CurrentUser.OpenSubKey ("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", true);
            if ((string) rkey.GetValue ("RyzenAdj") != null) {
                rkey.DeleteValue ("RyzenAdj");
            }
        }

        public void CreateBat () {
            int spm = 15;
            int sfl = 30;
            int ssl = 25;
            int tmp = 80;
            int vrm = 30000;

            if (rcheckBox1.Fill == Brushes.Green)
                spm = int.Parse (slider1.Value.ToString ());
            if (rcheckBox2.Fill == Brushes.Green)
                sfl = int.Parse (slider2.Value.ToString ());
            if (rcheckBox3.Fill == Brushes.Green)
                ssl = int.Parse (slider3.Value.ToString ());
            if (rcheckBox4.Fill == Brushes.Green)
                tmp = int.Parse (slider4.Value.ToString ());
            if (rcheckBox5.Fill == Brushes.Green)
                vrm = int.Parse (slider5.Value.ToString ());

            string vrmhex = vrm.ToString ("X");

            string bat = $"%~dp0\\ryzenadj.exe --stapm-limit={spm}000 --fast-limit={sfl}000 --slow-limit={ssl}000 --tctl-temp={tmp} --vrmmax-current=0x{vrmhex}";

            if (!File.Exists (batFilePath)) {
                using (FileStream fs = File.Create (batFilePath)) {
                    fs.Close ();
                }
            }

            using (StreamWriter sw = new StreamWriter (batFilePath)) {
                sw.WriteLine (bat);
            }
        }

        public void loadSettings ()
        {
            string contents = "";
            string[] values = null;
            if (File.Exists(batFilePath)) {
                contents = File.ReadAllText(batFilePath);
                values = contents.Split(new[] { "--" }, StringSplitOptions.None);
                // Separate the batch file into arguments @ values[]

                foreach (string s in values)
                {
                    //MessageBox.Show(s);
                    if (s.Contains("="))
                    {
                        string[] settingsArg = s.Split('=');
                        string type = settingsArg[0];
                        string typeValue = settingsArg[1];

                        switch (type)
                        {
                            case "stapm-limit":
                                rcheckBox1.Fill = Brushes.Green;
                                slider1.Value = double.Parse(typeValue);
                                cube1.Visibility = Visibility.Visible;
                                rect1s.Visibility = Visibility.Visible;
                                slider1.Visibility = Visibility.Visible;
                                lab1.Visibility = Visibility.Visible;
                                break;
                            case "fast-limit":
                                rcheckBox2.Fill = Brushes.Green;
                                slider2.Value = double.Parse(typeValue);
                                cube2.Visibility = Visibility.Visible;
                                rect2s.Visibility = Visibility.Visible;
                                slider2.Visibility = Visibility.Visible;
                                lab2.Visibility = Visibility.Visible;
                                break;
                            case "slow-limit":
                                rcheckBox3.Fill = Brushes.Green;
                                slider3.Value = double.Parse(typeValue);
                                cube3.Visibility = Visibility.Visible;
                                rect3s.Visibility = Visibility.Visible;
                                slider3.Visibility = Visibility.Visible;
                                lab3.Visibility = Visibility.Visible;
                                break;
                            case "tctl-temp":
                                rcheckBox4.Fill = Brushes.Green;
                                slider4.Value = double.Parse(typeValue);
                                cube4.Visibility = Visibility.Visible;
                                rect4s.Visibility = Visibility.Visible;
                                slider4.Visibility = Visibility.Visible;
                                lab4.Visibility = Visibility.Visible;
                                break;
                            case "vrmmax-current":
                                rcheckBox5.Fill = Brushes.Green;
                                slider5.Value = Convert.ToDouble(Convert.ToInt64(typeValue.Trim(), 16));
                                cube5.Visibility = Visibility.Visible;
                                rect5s.Visibility = Visibility.Visible;
                                slider5.Visibility = Visibility.Visible;
                                lab5.Visibility = Visibility.Visible;
                                break;
                            default:

                                break;
                        }
                        //MessageBox.Show(type);
                    }
                }
            }
        }

    }
}