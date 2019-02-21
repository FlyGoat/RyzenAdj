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

        public void RcheckBox7_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (rcheckBox7.Fill != Brushes.Green)
            {
                rcheckBox7.Fill = Brushes.Green;
                this.Height += 200;
            }
            else
            {
                rcheckBox7.Fill = Brushes.Transparent;
                this.Height -= 200;
            }
        }

        private void RcheckBox8_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (rcheckBox8.Fill != Brushes.Green)
            {
                rcheckBox8.Fill = Brushes.Green;
                rtextbox8.Visibility = Visibility.Visible;
            }
            else
            {
                rcheckBox8.Fill = Brushes.Transparent;
                rtextbox8.Visibility = Visibility.Hidden;
            }
        }

        private void RcheckBox9_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (rcheckBox9.Fill != Brushes.Green)
            {
                rcheckBox9.Fill = Brushes.Green;
                rtextbox9.Visibility = Visibility.Visible;
            }
            else
            {
                rcheckBox9.Fill = Brushes.Transparent;
                rtextbox9.Visibility = Visibility.Hidden;
            }
        }

        private void RcheckBox10_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (rcheckBox10.Fill != Brushes.Green)
            {
                rcheckBox10.Fill = Brushes.Green;
                rtextbox10.Visibility = Visibility.Visible;
            }
            else
            {
                rcheckBox10.Fill = Brushes.Transparent;
                rtextbox10.Visibility = Visibility.Hidden;
            }
        }

        private void RcheckBox11_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (rcheckBox11.Fill != Brushes.Green)
            {
                rcheckBox11.Fill = Brushes.Green;
                rtextbox11.Visibility = Visibility.Visible;
            }
            else
            {
                rcheckBox11.Fill = Brushes.Transparent;
                rtextbox11.Visibility = Visibility.Hidden;
            }
        }

        private void RcheckBox12_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (rcheckBox12.Fill != Brushes.Green)
            {
                rcheckBox12.Fill = Brushes.Green;
                rtextbox12.Visibility = Visibility.Visible;
            }
            else
            {
                rcheckBox12.Fill = Brushes.Transparent;
                rtextbox12.Visibility = Visibility.Hidden;
            }
        }

        private void RcheckBox13_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (rcheckBox13.Fill != Brushes.Green)
            {
                rcheckBox13.Fill = Brushes.Green;
                rtextbox13.Visibility = Visibility.Visible;
            }
            else
            {
                rcheckBox13.Fill = Brushes.Transparent;
                rtextbox13.Visibility = Visibility.Hidden;
            }
        }

        public void Apply_Click (object sender, RoutedEventArgs e) {
            if (RunExe ()) {
                MessageBox.Show ("Settings successfully applied!");
            } else {
                MessageBox.Show ("ERROR: Failed to apply settings.");
            }
            // System.Windows.Application.Current.Shutdown();
        }

        public void Exit_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Application.Current.Shutdown();
        }

        // Returns true if successful, else returns false.
        public bool RunExe () {
            int[] parameters_values = new int[11];
            int spm = -1;
            int sfl = -1;
            int ssl = -1;
            int tmp = -1;
            int vrm = -1;

            int spt = -1;
            int vsc = -1;
            int vcl = -1;
            int vsm = -1;
            int pcl = -1;
            int psc = -1;
        

            string[] parameter_names = { "stapm-limit", "fast-limit", "slow-limit", "tctl-temp", "vrmmax-current", "slow-time", "vrmsoc-current", "vrm-current", "vrmsocmax-current", "psi0-current", "psi0soc-current" };
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
            if (rcheckBox8.Fill == Brushes.Green && rtextbox8.Text.Trim() != "")
                spt = int.Parse(rtextbox8.Text);
            if (rcheckBox9.Fill == Brushes.Green && rtextbox9.Text.Trim() != "")
                vsc = int.Parse(rtextbox9.Text);
            if (rcheckBox10.Fill == Brushes.Green && rtextbox10.Text.Trim() != "")
                vcl = int.Parse(rtextbox10.Text);
            if (rcheckBox11.Fill == Brushes.Green && rtextbox11.Text.Trim() != "")
                vsm = int.Parse(rtextbox11.Text);
            if (rcheckBox12.Fill == Brushes.Green && rtextbox12.Text.Trim() != "")
                pcl = int.Parse(rtextbox12.Text);
            if (rcheckBox13.Fill == Brushes.Green && rtextbox13.Text.Trim() != "")
                psc = int.Parse(rtextbox13.Text);

            parameters_values[0] = spm;
            parameters_values[1] = sfl;
            parameters_values[2] = ssl;
            parameters_values[3] = tmp;
            parameters_values[4] = vrm;
            parameters_values[5] = spt;
            parameters_values[6] = vsc;
            parameters_values[7] = vcl;
            parameters_values[8] = vsm;
            parameters_values[9] = pcl;
            parameters_values[10] = psc;

            string args = $"";

            int counter = 0;
            foreach (string key in parameter_names)
            {
                if (parameters_values[counter] == -1)
                {
                }
                else if (key == "stapm-limit" || key == "fast-limit" || key == "slow-limit")
                {
                    //MessageBox.Show(key + ": " + parameters_values[counter]);
                    args += $" --{key}={parameters_values[counter]}000";
                }
                else if (key == "tctl-temp" || key == "slow-time")
                {
                    //MessageBox.Show(key + ": " + parameters_values[counter]);
                    args += $" --{key}={parameters_values[counter]}";
                }
                else
                {
                    string hex = parameters_values[counter].ToString("X");
                    //MessageBox.Show(key + ": " + hex);
                    args += $" --{key}=0x{hex}";
                }
                counter++;
            }

            MessageBoxResult dialogResult = MessageBox.Show("Execute the following args: \n" + args, "Are you sure?", MessageBoxButton.YesNo);
            if (dialogResult == MessageBoxResult.Yes)
            {
                try
                {
                    Process.Start (new ProcessStartInfo {
                      FileName = $"{dir}\\ryzenadj.exe",
                        UseShellExecute = false,
                      CreateNoWindow = true,
                    Arguments = args,
                    RedirectStandardOutput = true
                    });
                }
                catch
                {
                    MessageBox.Show("Can't find ryzenadj.exe folder");
                    return false;
                }
                CreateBat();
                if (rcheckBox6.Fill == Brushes.Green)
                {
                    AutoStart();
                }
                else
                {
                    ClearAutoStart();
                }
            }
            else if (dialogResult == MessageBoxResult.No)
            {
                MessageBox.Show("Aborted.");
                return false;
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
            int[] parameters_values = new int[11];
            int spm = -1;
            int sfl = -1;
            int ssl = -1;
            int tmp = -1;
            int vrm = -1;

            int spt = -1;
            int vsc = -1;
            int vcl = -1;
            int vsm = -1;
            int pcl = -1;
            int psc = -1;
            

            string[] parameter_names = { "stapm-limit", "fast-limit", "slow-limit", "tctl-temp", "vrmmax-current", "slow-time", "vrmsoc-current", "vrm-current", "vrmsocmax-current", "psi0-current", "psi0soc-current" };
            if (rcheckBox1.Fill == Brushes.Green)
                spm = int.Parse(slider1.Value.ToString());
            if (rcheckBox2.Fill == Brushes.Green)
                sfl = int.Parse(slider2.Value.ToString());
            if (rcheckBox3.Fill == Brushes.Green)
                ssl = int.Parse(slider3.Value.ToString());
            if (rcheckBox4.Fill == Brushes.Green)
                tmp = int.Parse(slider4.Value.ToString());
            if (rcheckBox5.Fill == Brushes.Green)
                vrm = int.Parse(slider5.Value.ToString());
            if (rcheckBox8.Fill == Brushes.Green)
                spt = int.Parse(rtextbox8.Text);
            if (rcheckBox9.Fill == Brushes.Green)
                vsc = int.Parse(rtextbox9.Text);
            if (rcheckBox10.Fill == Brushes.Green)
                vcl = int.Parse(rtextbox10.Text);
            if (rcheckBox11.Fill == Brushes.Green)
                vsm = int.Parse(rtextbox11.Text);
            if (rcheckBox12.Fill == Brushes.Green)
                pcl = int.Parse(rtextbox12.Text);
            if (rcheckBox13.Fill == Brushes.Green)
                psc = int.Parse(rtextbox13.Text);

            parameters_values[0] = spm;
            parameters_values[1] = sfl;
            parameters_values[2] = ssl;
            parameters_values[3] = tmp;
            parameters_values[4] = vrm;
            parameters_values[5] = spt;
            parameters_values[6] = vsc;
            parameters_values[7] = vcl;
            parameters_values[8] = vsm;
            parameters_values[9] = pcl;
            parameters_values[10] = psc;

            string bat = $"%~dp0\\ryzenadj.exe";

            int counter = 0;
            foreach (string key in parameter_names)
            {
                
                if (parameters_values[counter] == -1)
                {
                    //MessageBox.Show(key + " is missing at counter " + counter);
                }
                else if (key == "stapm-limit" || key == "fast-limit" || key == "slow-limit")
                {
                    //MessageBox.Show(key + ": " + parameters_values[counter]);
                    bat += $" --{key}={parameters_values[counter]}000";
                }
                else if (key == "tctl-temp" || key == "slow-time")
                {
                    //MessageBox.Show(key + ": " + parameters_values[counter]);
                    bat += $" --{key}={parameters_values[counter]}";
                }
                else
                {
                    string hex = parameters_values[counter].ToString("X");
                    //MessageBox.Show(key + ": " + hex);
                    bat += $" --{key}=0x{hex}";
                }
                counter++;
            }


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
                                slider1.Value = double.Parse(typeValue.Substring(0, typeValue.Length - 4));
                                cube1.Visibility = Visibility.Visible;
                                rect1s.Visibility = Visibility.Visible;
                                slider1.Visibility = Visibility.Visible;
                                lab1.Visibility = Visibility.Visible;
                                break;
                            case "fast-limit":
                                rcheckBox2.Fill = Brushes.Green;
                                slider2.Value = double.Parse(typeValue.Substring(0, typeValue.Length - 4));
                                cube2.Visibility = Visibility.Visible;
                                rect2s.Visibility = Visibility.Visible;
                                slider2.Visibility = Visibility.Visible;
                                lab2.Visibility = Visibility.Visible;
                                break;
                            case "slow-limit":
                                rcheckBox3.Fill = Brushes.Green;
                                slider3.Value = double.Parse(typeValue.Substring(0, typeValue.Length - 4));
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
                            case "slow-time":
                                if (rcheckBox7.Fill != Brushes.Green)
                                {
                                    RcheckBox7_MouseDown(null, null);
                                }
                                rcheckBox8.Fill = Brushes.Green;
                                rtextbox8.Text = typeValue.Trim();
                                rtextbox8.Visibility = Visibility.Visible;
                                break;
                            case "vrmsoc-current":
                                if (rcheckBox7.Fill != Brushes.Green)
                                {
                                    RcheckBox7_MouseDown(null, null);
                                }
                                rcheckBox9.Fill = Brushes.Green;
                                rtextbox9.Text = Convert.ToString(Convert.ToDouble(Convert.ToInt64(typeValue.Trim(), 16)));
                                rtextbox9.Visibility = Visibility.Visible;
                                break;
                            case "vrm-current":
                                if (rcheckBox7.Fill != Brushes.Green)
                                {
                                    RcheckBox7_MouseDown(null, null);
                                }
                                rcheckBox10.Fill = Brushes.Green;
                                rtextbox10.Text = Convert.ToString(Convert.ToDouble(Convert.ToInt64(typeValue.Trim(), 16)));
                                rtextbox10.Visibility = Visibility.Visible;
                                break;
                            case "vrmsocmax-current":
                                if (rcheckBox7.Fill != Brushes.Green)
                                {
                                    RcheckBox7_MouseDown(null, null);
                                }
                                rcheckBox11.Fill = Brushes.Green;
                                rtextbox11.Text = Convert.ToString(Convert.ToDouble(Convert.ToInt64(typeValue.Trim(), 16)));
                                rtextbox11.Visibility = Visibility.Visible;
                                break;
                            case "psi0-current":
                                if (rcheckBox7.Fill != Brushes.Green)
                                {
                                    RcheckBox7_MouseDown(null, null);
                                }
                                rcheckBox12.Fill = Brushes.Green;
                                rtextbox12.Text = Convert.ToString(Convert.ToDouble(Convert.ToInt64(typeValue.Trim(), 16)));
                                rtextbox12.Visibility = Visibility.Visible;
                                break;
                            case "psi0soc-current":
                                if (rcheckBox7.Fill != Brushes.Green)
                                {
                                    RcheckBox7_MouseDown(null, null);
                                }
                                rcheckBox13.Fill = Brushes.Green;
                                rtextbox13.Text = Convert.ToString(Convert.ToDouble(Convert.ToInt64(typeValue.Trim(), 16)));
                                rtextbox13.Visibility = Visibility.Visible;
                                break;
                            default:
                                MessageBox.Show("WARN: Unsupported settings value: " + type + " with the value of: " + typeValue + " has been ignored.");
                                break;
                        }
                        //MessageBox.Show(type);
                    }
                }
            }
        }

        
    }
}