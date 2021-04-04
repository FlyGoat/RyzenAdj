<#
.SYNOPSIS
    Automates ryzenAdj calls based on custom conditions
.DESCRIPTION
    This script is designed to provide maximum flexibility to the user. For that reason it does not use parameters.
    Instead of parameters, you need to populate the functions in the configuration section with your custom adjustments and additional custom code.
.NOTES
    SPDX-License-Identifier: LGPL
    Falco Schaffrath <falco.schaffrath@gmail.com>
#>

Param([Parameter(Mandatory=$false)][switch]$noGUI)

#### Configuration Start

# WARNING: Use at your own risk!

$pathToRyzenAdjDlls = Split-Path -Parent $PSCommandPath #script path is DLL path, needs to be absolut path if you define something else

$showErrorPopupsDuringInit = $true
# debug mode prints adjust success messages too instead of errorss only
$debugMode = $false
# if monitorField is set, this script does only adjust values if something did revert your monitored value. Clear monitorField String to disable monitoring
# This needs to be an value which actually gets overwritten by your device firmware/software if no changes get detected, your settings will not reapplied
# Don't use this feature if your device does not exetly apply your values. For example don't use it if setting fast-limit 40W does result into 36W fast-limit
$monitorField = "fast_limit"

function doAdjust_ACmode {
    $Script:repeatWaitTimeSeconds = 3    #reapplies setting every 3s
    adjust "fast_limit" 46000
    adjust "slow_limit" 25000
    #adjust "slow_time" 30
    #adjust "tctl_temp" 95
    #adjust "apu_skin_temp_limit" 50
    #adjust "vrmmax_current" 100000
    #adjust "<any_other_field>" 1234

    #custom code, for example set fan controll back to auto
    #values (WriteRegister: 47, FanSpeedResetValue:128) extracted from similar devices at https://github.com/hirschmann/nbfc/blob/master/Configs/
    #Start-Process -NoNewWindow -Wait -filePath "C:\Program Files (x86)\NoteBook FanControl\ec-probe.exe" -ArgumentList("write", "47", "128")
}

function doAdjust_BatteryMode {
    $Script:repeatWaitTimeSeconds = 10   #do less reapplies to save power
    adjust "fast_limit" 26000
    adjust "slow_limit" 10000

    #custom code, for example disable fan to save power
    #Start-Process -NoNewWindow -Wait -filePath "C:\Program Files (x86)\NoteBook FanControl\ec-probe.exe" -ArgumentList("write", "47", "0")
}

#### Configuration End
################################################################################

$env:PATH += ";$pathToRyzenAdjDlls"
$NL = $([System.Environment]::NewLine);

if($noGUI){ $showErrorPopupsDuringInit = $false }

$apiHeader = @'
[DllImport("libryzenadj.dll")] public static extern IntPtr init_ryzenadj();
[DllImport("libryzenadj.dll")] public static extern int set_stapm_limit(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_fast_limit(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_slow_limit(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_slow_time(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_stapm_time(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_tctl_temp(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_vrm_current(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_vrmsoc_current(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_vrmmax_current(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_vrmsocmax_current(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_psi0_current(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_psi0soc_current(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_gfxclk_freq(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_gfxclk_freq(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_socclk_freq(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_socclk_freq(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_fclk_freq(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_fclk_freq(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_vcn(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_vcn(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_max_lclk(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_min_lclk(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_prochot_deassertion_ramp(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_apu_skin_temp_limit(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_dgpu_skin_temp_limit(IntPtr ry, [In]uint value);
[DllImport("libryzenadj.dll")] public static extern int set_apu_slow_limit(IntPtr ry, [In]uint value);

[DllImport("libryzenadj.dll")] public static extern int refresh_table(IntPtr ry);

[DllImport("libryzenadj.dll")] public static extern IntPtr get_table_values(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_fast_limit(IntPtr ry);

[DllImport("kernel32.dll")] public static extern uint GetModuleFileName(IntPtr hModule, [Out]StringBuilder lpFilename, [In]int nSize);
[DllImport("kernel32.dll")] public static extern Boolean GetSystemPowerStatus(out SystemPowerStatus sps);
public struct SystemPowerStatus {
  public Byte ACLineStatus;
  public Byte BatteryFlag;
  public Byte BatteryLifePercent;
  public Byte Reserved1;
  public Int32 BatteryLifeTime;
  public Int32 BatteryFullLifeTime;
}

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
        [void][System.Windows.Forms.MessageBox]::Show($msg, $PSCommandPath,
        [System.Windows.Forms.MessageBoxButtons]::OK,
        [System.Windows.Forms.MessageBoxIcon]::Error)
    }
}

$dllImportErrors = [ryzen.adj]::getDllImportErrors();
if($dllImportErrors -or $Error){
    Write-Error $dllImportErrors
    showErrorMsg "Problem with using libryzenadj.dll$NL$NL$($Error -join $NL)"
    exit 1
}

$winring0DriverFilepath = [ryzen.adj]::getExpectedWinRing0DriverFilepath()
if(!(Test-Path $winring0DriverFilepath)) { Copy-Item -Path $pathToRyzenAdjDlls\WinRing0x64.sys -Destination $winring0DriverFilepath }

$ry = [ryzen.adj]::init_ryzenadj()
if($ry -eq 0){
    $msg = "RyzenAdj could not get initialized.$($NL)Reason can be found inside Powershell$($NL)"
    if($psISE) { $msg += "It is not possible to see the error reason inside ISE, you need to test it in PowerShell Console" }
    showErrorMsg "$msg$NL$NL$($Error -join $NL)"
    exit 1
}

function adjust ([String] $fieldName, [uInt32] $value) {
    if($fieldName -eq $Script:monitorField) {
        $newTargetValue = [math]::floor($value / 1000)
        if($Script:targetMonitorValue -ne $newTargetValue){
            $Script:targetMonitorValue = $newTargetValue
            Write-Host "set new monitoring target $fieldName to $newTargetValue"
        }
    }

    $res = Invoke-Expression "[ryzen.adj]::set_$fieldName($ry, $value)"
    switch ($res) {
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

function testMonitorField {
    if($monitorField){
        [void][ryzen.adj]::refresh_table($ry)
        $monitorValue = [math]::floor((getMonitorValue))
        if($Script:targetMonitorValue -ne $monitorValue){
            Write-Error ("Value Monitoring does not work, should be '$Script:targetMonitorValue' but was '$monitorValue'.$NL$NL" +
                "If you ignore it, the script will apply values unnessasary often.$NL")
        }
    }
}

function testAdjustments {
    Write-Host "Test Adjustments"
    if($Script:systemPowerStatus.ACLineStatus){
        doAdjust_BatteryMode
        testMonitorField
        doAdjust_ACmode
    } else {
        doAdjust_ACmode
        testMonitorField
        doAdjust_BatteryMode
    }
    testMonitorField

    if($Error -and $showErrorPopupsDuringInit){
        $answer = [System.Windows.Forms.MessageBox]::Show("Your Adjustment configuration did not work.$NL$NL$($Error -join $NL)", $PSCommandPath,
            [System.Windows.Forms.MessageBoxButtons]::AbortRetryIgnore,
            [System.Windows.Forms.MessageBoxIcon]::Warning)
        $Error.Clear()
        if($answer -eq "Abort"){ exit 1 }
        if($answer -eq "Retry"){ testAdjustments }
    }
}

function getMonitorValue {
    if($monitorField){
        return Invoke-Expression "[ryzen.adj]::get_$monitorField($ry)"
    }
    return 0
}

if(-not $Script:repeatWaitTimeSeconds) { $Script:repeatWaitTimeSeconds = 5 }
$Script:targetMonitorValue = 0;

$systemPowerStatus = New-Object ryzen.adj+SystemPowerStatus
[void][ryzen.adj]::GetSystemPowerStatus([ref]$systemPowerStatus)

testAdjustments

<# Example how to get first 100 lines of ptable
$pmTable = [float[]]::new(100)
$tablePtr = [ryzen.adj]::get_table_values($ry);
[System.Runtime.InteropServices.Marshal]::Copy($tablePtr, $pmTable, 0, 100);
$pmTable
#>

$processType = "Apply Settings"
if($monitorField){
    $processType = "Monitor $monitorField changes"
}

Write-Host "$processType every $Script:repeatWaitTimeSeconds seconds..."
while($true) {
    if($monitorField) {
        [void][ryzen.adj]::refresh_table($ry)
    }
    if(!$monitorField -or $Script:targetMonitorValue -ne [math]::floor((getMonitorValue))){
        [void][ryzen.adj]::GetSystemPowerStatus([ref]$systemPowerStatus)
        $oldWait = $Script:repeatWaitTimeSeconds
        if($systemPowerStatus.ACLineStatus){
            doAdjust_ACmode
        } else {
            doAdjust_BatteryMode
        }
        if($oldWait -ne $Script:repeatWaitTimeSeconds ) { Write-Host "$processType every $Script:repeatWaitTimeSeconds seconds..." }
    }
    sleep $Script:repeatWaitTimeSeconds
}
