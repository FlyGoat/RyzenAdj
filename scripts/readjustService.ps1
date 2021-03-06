Param([Parameter(Mandatory=$false)][switch]$noGUI)

#### Configuration Start

$pathToRyzenAdjDlls = Split-Path -Parent $PSCommandPath #script path is DLL path, needs to be absolut path if you define something else

$showErrorPopupsDuringInit = $true

$debugMode = $false                      #prints adjust success messages too instead of errorss only

function doAdjust_ACmode {
    adjust "fast_limit" 45000
    adjust "slow_limit" 25000
    #adjust "slow_time" 30
    #adjust "tctl_temp" 97
    #adjust "apu_skin_temp_limit" 50
    #adjust "vrmmax_current" 100000
    $Script:repeatWaitTimeSeconds = 3    #reapplies setting every 3s
}

function doAdjust_BatteryMode {
    adjust "fast_limit" 25000
    adjust "slow_limit" 10000
    $Script:repeatWaitTimeSeconds = 30   #do less reapplies to save power
}

#### Configuration End
################################################################################

$env:PATH += ";$pathToRyzenAdjDlls"
$NL = $([System.Environment]::NewLine);

if($noGUI){ $showErrorPopupsDuringInit = $false }

$apiHeader = @'
[DllImport("libryzenadj.dll")] public static extern IntPtr init_ryzenadj();
[DllImport("libryzenadj.dll")] public static extern int set_stapm_limit(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_fast_limit(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_slow_limit(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_slow_time(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_stapm_time(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_tctl_temp(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_vrm_current(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_vrmsoc_current(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_vrmmax_current(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_vrmsocmax_current(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_psi0_current(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_psi0soc_current(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_gfxclk_freq(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_gfxclk_freq(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_socclk_freq(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_socclk_freq(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_fclk_freq(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_fclk_freq(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_vcn(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_vcn(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_lclk(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_lclk(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_prochot_deassertion_ramp(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_apu_skin_temp_limit(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_dgpu_skin_temp_limit(IntPtr ry, uint value);
[DllImport("libryzenadj.dll")] public static extern int set_apu_slow_limit(IntPtr ry, uint value);

[DllImport("kernel32.dll")] public static extern uint GetModuleFileName(IntPtr hModule, [Out]StringBuilder lpFilename, [In]int nSize);

public static String getExpectedWinRing0DriverFilepath(){
    StringBuilder fileName = new StringBuilder(255);
    GetModuleFileName(IntPtr.Zero, fileName, fileName.Capacity);
    return Path.GetDirectoryName(fileName.ToString()) + "\\WinRing0x64.sys";
}

public static String getDllImportErrors(){
    try {
        Marshal.PrelinkAll(typeof(adj));
    } catch (Exception e) {
        return e.Message;
    }
    return "";
}
'@

if(-not ([System.Management.Automation.PSTypeName]'ryzen.adj').Type){
    Add-Type -MemberDefinition $apiHeader -Namespace 'ryzen' -Name 'adj' -UsingNamespace ('System.Text', 'System.IO')
}

Add-Type -AssemblyName System.Windows.Forms
function showErrorMsg ([String] $msg){
    if($showErrorPopupsDuringInit){
        $res = [System.Windows.Forms.MessageBox]::Show($msg, $PSCommandPath,
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Error)
    }
}

$dllImportErrors = [ryzen.adj]::getDllImportErrors();
if($dllImportErrors -or $Error){
    Write-Error $dllImportErrors
    showErrorMsg "Problem with using libryzenadj.dll$NL$NL$Error"
    exit 1
}

$winring0DriverFilepath = [ryzen.adj]::getExpectedWinRing0DriverFilepath()
if(!(Test-Path $winring0DriverFilepath)) { Copy-Item -Path $pathToRyzenAdjDlls\WinRing0x64.sys -Destination $winring0DriverFilepath }

$ry = [ryzen.adj]::init_ryzenadj()
if($ry -eq 0){
    $msg = "RyzenAdj could not get initialized.$($NL)Reason can be found inside Powershell$($NL)"
    if($psISE) { $msg += "It is not possible to see the error reason inside ISE, you need to test it in PowerShell Console" }
    showErrorMsg "$msg$NL$NL$Error"
    exit 1
}

function adjust ([String] $fieldName, [uInt32] $value) {
    $res = Invoke-Expression "[ryzen.adj]::set_$fieldName($ry, $value)"
    switch ($res)
    {
        0 {
            if($debugMode) { Write-Host "set $fieldName to $value" }
            return
        }
        -1 { Write-Error "set_$fieldName is not supported on this family"}
        -3 { Write-Error "set_$fieldName is not supported on this SMU"}
        -4 { Write-Error "set_$fieldName is rejected by SMU"}
        default { Write-Error "set_$fieldName did fail with $res"}
    }
}

function testAdjustments {
    Write-Host "Test Adjustments"
    $batteryStatus=(Get-WmiObject -Class BatteryStatus -Namespace root\wmi -ErrorAction Ignore)
    if($batteryStatus){
        $isDesktop = $false;
        if($batteryStatus.PowerOnLine){
            doAdjust_BatteryMode
            doAdjust_ACmode
        } else {
            doAdjust_ACmode
            doAdjust_BatteryMode
        }
    } else {
        $isDesktop = $true;
        Write-Host "Device has no battery, doAdjust_BatteryMode will be ignored"
        doAdjust_ACmode
    }
    if($Error -and $showErrorPopupsDuringInit){
        $answer = [System.Windows.Forms.MessageBox]::Show("Your Adjustment configuration did not work.$NL$NL$Error", $PSCommandPath,
            [System.Windows.Forms.MessageBoxButtons]::AbortRetryIgnore,
            [System.Windows.Forms.MessageBoxIcon]::Warning)
        $Error.Clear()
        if($answer -eq "Abort"){ exit 1 }
        if($answer -eq "Retry"){ testAdjustments }
    }
}

if(-not $Script:repeatWaitTimeSeconds) { $Script:repeatWaitTimeSeconds = 5 }

testAdjustments

Write-Host "Apply settings every $Script:repeatWaitTimeSeconds seconds..."
while($true) {
    $oldWait = $Script:repeatWaitTimeSeconds
    if($isDesktop -or (Get-WmiObject -Class BatteryStatus -Namespace root\wmi).PowerOnLine){
        doAdjust_ACmode
    } else {
        doAdjust_BatteryMode
    }
    if($oldWait -ne $Script:repeatWaitTimeSeconds ) { Write-Host "Apply settings every $Script:repeatWaitTimeSeconds seconds..." }
    sleep $Script:repeatWaitTimeSeconds
}
