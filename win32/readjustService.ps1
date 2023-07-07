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
$Error.Clear()
################################################################################
#### Configuration Start
################################################################################
# WARNING: Use at your own risk!

$pathToRyzenAdjDlls = Split-Path -Parent $PSCommandPath #script path is DLL path, needs to be absolut path if you define something else

$showErrorPopupsDuringInit = $true
# debug mode prints adjust success messages too instead of errorss only
$debugMode = $false
# if monitorField is set, this script does only adjust values if something did revert your monitored value. Clear monitorField String to disable monitoring
# This needs to be an value which actually gets overwritten by your device firmware/software if no changes get detected, your settings will not reapplied
$monitorField = "fast_limit"
# Does reapply adjustments if power slider did change position, check $Script:acSlider or $Script:dcSlider to apply slider specific values
$monitorPowerSlider = $true
# Does reapply adjustments if AC line status did change when AC power cable is plugged or unplugged
$monitorACLineStatus = $true
# HWiNFO needs to be restartet after this script did run the first time with this option
$updateHWINFOSensors = $false
# some Zen3 devices have a locked STAPM limit, this workarround resets the stapm timer to have unlimited stapm. Use max stapm_limit and stapm_time (usually 500) to triger as less resets as possible
$resetSTAPMUsage = $false

function doAdjust_ACmode {
    $Script:repeatWaitTimeSeconds = 1    #only use values below 5s if you are using $monitorField
    adjust "fast_limit" 46000
    adjust "slow_limit" 25000
    #adjust "slow_time" 30
    #adjust "tctl_temp" 93
    #adjust "apu_skin_temp_limit" 50
    #adjust "vrmmax_current" 100000
    #replace any_other_field with additional adjustments. Name is equal to RyzenAdj options but it uses _ instead of -
    #adjust "any_other_field" 1234

    #custom code, for example set fan controll back to auto
    #values (WriteRegister: 47, FanSpeedResetValue:128) extracted from similar devices at https://github.com/hirschmann/nbfc/blob/master/Configs/
    #Start-Process -NoNewWindow -Wait -filePath "C:\Program Files (x86)\NoteBook FanControl\ec-probe.exe" -ArgumentList("write", "47", "128")

    if($Script:acSlider -eq $Script:betterBattery){
        #put adjustments for energie saving slider position here:
        enable "power_saving" #add 10s boost delay for usage on cable to reduce idle power consumtion
    }
}

function doAdjust_BatteryMode {
    $Script:repeatWaitTimeSeconds = 10   #do less reapplies and less HWiNFO updates to save power
    adjust "fast_limit" 26000
    adjust "slow_limit" 10000
    #adjust "any_other_field" 1234

    if($Script:dcSlider -eq $Script:betterBattery){
        #put adjustments for energie saving slider position here: for example disable fan to save power
        #Start-Process -NoNewWindow -Wait -filePath "C:\Program Files (x86)\NoteBook FanControl\ec-probe.exe" -ArgumentList("write", "47", "0")
    }

    if($Script:dcSlider -eq $Script:bestPerformance){
        #put adjustments for highest performance slider position here:
        enable "max_performance" #removes 10s boost delay on battery
        doAdjust_ACmode #set limits from cable mode on battery
    }
}
################################################################################
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
[DllImport("libryzenadj.dll")] public static extern int set_power_saving(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern int set_max_performance(IntPtr ry);

[DllImport("libryzenadj.dll")] public static extern int refresh_table(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern IntPtr get_table_values(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_stapm_limit(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_stapm_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_stapm_time(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_fast_limit(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_fast_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_slow_limit(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_slow_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_apu_slow_limit(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_apu_slow_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_vrm_current(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_vrm_current_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_vrmsoc_current(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_vrmsoc_current_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_vrmmax_current(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_vrmmax_current_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_vrmsocmax_current(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_vrmsocmax_current_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_tctl_temp(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_tctl_temp_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_apu_skin_temp_limit(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_apu_skin_temp_value(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_dgpu_skin_temp_limit(IntPtr ry);
[DllImport("libryzenadj.dll")] public static extern float get_dgpu_skin_temp_value(IntPtr ry);

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
        $newTargetValue = [math]::round($value * 0.001, 3, 0)
        if($Script:monitorFieldAdjTarget -ne $newTargetValue){
            $Script:monitorFieldAdjTarget = $newTargetValue
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

function enable ([String] $fieldName) {
    $res = Invoke-Expression "[ryzen.adj]::set_$fieldName($ry)"
    switch ($res) {
        0 {
            if($debugMode) { Write-Host "enable $fieldName"}
            return
        }
        -1 { Write-Error "set_$fieldName is not supported on this family"}
        -3 { Write-Error "set_$fieldName is not supported on this SMU"}
        -4 { Write-Error "set_$fieldName is rejected by SMU"}
        default { Write-Error "set_$fieldName did fail with $res"}
    }
}

function testMonitorField {
    if($monitorField -and $Script:monitorFieldAdjTarget -eq 0){
        Write-Error ("You forgot to set $monitorField in your profile.$NL$NL" +
            "If you ignore it, the script will apply values unnessasary often.$NL")
    }
}

function updateMonitorFieldAdjResult {
    if($monitorField){
        [void][ryzen.adj]::refresh_table($ry)
        $Script:monitorFieldAdjResult = [math]::round((getMonitorValue), 3, 0)
        if($Script:monitorFieldAdjTarget -ne $Script:monitorFieldAdjResult){
            Write-Host ("Warning - $monitorField adjust result $Script:monitorFieldAdjResult does not match target value $Script:monitorFieldAdjTarget. Value $Script:monitorFieldAdjResult will be used for monitoring")
        }
    }
}

function testConfiguration {
    Write-Host "Test Adjustments"
    if($Script:systemPowerStatus.ACLineStatus){
        doAdjust_BatteryMode
        testMonitorField
        $Script:monitorFieldAdjTarget = 0
        doAdjust_ACmode
    } else {
        doAdjust_ACmode
        testMonitorField
        $Script:monitorFieldAdjTarget = 0
        doAdjust_BatteryMode
    }
    testMonitorField
    updateMonitorFieldAdjResult

    if($resetSTAPMUsage -and [ryzen.adj]::get_stapm_time($ry) -eq 1) {
        Write-Error "resetSTAPMUsage function does only work on devices with active STAPM control and don't do anything on devices with enabled STT control."
    }

    if($Error -and $showErrorPopupsDuringInit){
        $answer = [System.Windows.Forms.MessageBox]::Show("Your Adjustment configuration did not work.$NL$NL$($Error -join $NL)", $PSCommandPath,
            [System.Windows.Forms.MessageBoxButtons]::AbortRetryIgnore,
            [System.Windows.Forms.MessageBoxIcon]::Warning)
        $Error.Clear()
        if($answer -eq "Abort"){ exit 1 }
        if($answer -eq "Retry"){ testConfiguration }
    }
}

function getMonitorValue {
    if($monitorField){
        return Invoke-Expression "[ryzen.adj]::get_$monitorField($ry)"
    }
    return 0
}

function createOrDeleteHWINFOSensors {
    if($updateHWINFOSensors){
        New-Item -Path HKCU:\Software\HWiNFO64\Sensors\Custom -Name RyzenAdj -Force > $null
        'Key,Name,Value
        Power0,STAPM Limit
        Power1,STAPM
        Power2,PPT FAST Limit
        Power3,PPT FAST
        Power4,PPT SLOW Limit
        Power5,PPT SLOW
        Power6,APU SLOW Limit
        Power7,APU SLOW
        Current0,TDC VRM Limit
        Current1,TDC VRM
        Current2,TDC VRM SoC Limit
        Current3,TDC VRM SoC
        Current4,EDC VRM Max Limit
        Current5,EDC VRM Max
        Current6,EDC VRM SoC Max Limit
        Current7,EDC VRM SoC Max
        Temp0,TCTL Temp Limit
        Temp1,TCTL Temp
        Temp2,SST APU Skin Temp Limit
        Temp3,SST APU Skin Temp
        Temp4,SST dGPU Skin Temp Limit
        Temp5,SST dGPU Skin Temp
        Usage0,STAPM Limit Usage, ("STAPM" / "STAPM Limit") * 100
        Usage1,PPT FAST Limit Usage, ("PPT FAST" / "PPT FAST Limit") * 100
        Usage2,PPT SLOW Limit Usage, ("PPT SLOW" / "PPT SLOW Limit") * 100
        Usage3,APU SLOW Limit Usage, ("APU SLOW" / "APU SLOW Limit") * 100
        Usage4,TDC VRM Limit Usage, ("TDC VRM" / "TDC VRM Limit") * 100
        Usage5,TDC VRM SoC Limit Usage, ("TDC VRM SoC" / "TDC VRM SoC Limit") * 100
        Usage6,EDC VRM Max Limit Usage, ("EDC VRM Max" / "EDC VRM Max Limit") * 100
        Usage7,EDC VRM SoC Max Limit Usage, ("EDC VRM SoC Max" / "EDC VRM SoC Max Limit") * 100
        Usage8,TCTL Temp Limit Usage, ("TCTL Temp" / "TCTL Temp Limit") * 100
        Usage9,SST APU Skin Temp Limit Usage, ("SST APU Skin Temp" / "SST APU Skin Temp Limit") * 100
        Usage10,SST dGPU Skin Temp Limit Usage, ("SST dGPU Skin Temp" / "SST dGPU Skin Temp Limit") * 100' | ConvertFrom-Csv -outvariable hwinfo_keys > $null
        $hwinfo_keys | ForEach-Object {New-item -Path HKCU:\Software\HWiNFO64\Sensors\Custom\RyzenAdj -Name $_.Key -Force} > $null
        $hwinfo_keys | ForEach-Object {[Microsoft.Win32.Registry]::SetValue("HKEY_CURRENT_USER\Software\HWiNFO64\Sensors\Custom\RyzenAdj\" + $_.Key,"Name",$_.Name)} > $null
        $hwinfo_keys | Where Value -ne $null | ForEach-Object {[Microsoft.Win32.Registry]::SetValue("HKEY_CURRENT_USER\Software\HWiNFO64\Sensors\Custom\RyzenAdj\" + $_.Key,"Value",$_.Value)} > $null
    } else {
        Remove-Item HKCU:\Software\HWiNFO64\Sensors\Custom\RyzenAdj -Recurse -ErrorAction:Ignore
    }
}

function setHWINFOValue ([String] $name, [float] $value) {
    if(![float]::IsNaN($value)){ [Microsoft.Win32.Registry]::SetValue("HKEY_CURRENT_USER\Software\HWiNFO64\Sensors\Custom\RyzenAdj\" + $name,"Value",[String]$value) }
}

function updateHWINFOSensors {
    setHWINFOValue Power0   ([ryzen.adj]::get_stapm_limit($ry))
    setHWINFOValue Power1   ([ryzen.adj]::get_stapm_value($ry))
    setHWINFOValue Power2   ([ryzen.adj]::get_fast_limit($ry))
    setHWINFOValue Power3   ([ryzen.adj]::get_fast_value($ry))
    setHWINFOValue Power4   ([ryzen.adj]::get_slow_limit($ry))
    setHWINFOValue Power5   ([ryzen.adj]::get_slow_value($ry))
    setHWINFOValue Power6   ([ryzen.adj]::get_apu_slow_limit($ry))
    setHWINFOValue Power7   ([ryzen.adj]::get_apu_slow_value($ry))
    setHWINFOValue Current0 ([ryzen.adj]::get_vrm_current($ry))
    setHWINFOValue Current1 ([ryzen.adj]::get_vrm_current_value($ry))
    setHWINFOValue Current2 ([ryzen.adj]::get_vrmsoc_current($ry))
    setHWINFOValue Current3 ([ryzen.adj]::get_vrmsoc_current_value($ry))
    setHWINFOValue Current4 ([ryzen.adj]::get_vrmmax_current($ry))
    setHWINFOValue Current5 ([ryzen.adj]::get_vrmmax_current_value($ry))
    setHWINFOValue Current6 ([ryzen.adj]::get_vrmsocmax_current($ry))
    setHWINFOValue Current7 ([ryzen.adj]::get_vrmsocmax_current_value($ry))
    setHWINFOValue Temp0    ([ryzen.adj]::get_tctl_temp($ry))
    setHWINFOValue Temp1    ([ryzen.adj]::get_tctl_temp_value($ry))
    setHWINFOValue Temp2    ([ryzen.adj]::get_apu_skin_temp_limit($ry))
    setHWINFOValue Temp3    ([ryzen.adj]::get_apu_skin_temp_value($ry))
    setHWINFOValue Temp4    ([ryzen.adj]::get_dgpu_skin_temp_limit($ry))
    setHWINFOValue Temp5    ([ryzen.adj]::get_dgpu_skin_temp_value($ry))
    #setHWINFOValue Usage11 $pmTable[546]
}

function resetSTAPMIfNeeded {
    $stapm_limit = [ryzen.adj]::get_stapm_limit($ry)
    $stapm_value = [ryzen.adj]::get_stapm_value($ry)
    $stapm_hysteresis = 1 #Throttling starts arround ~0.9W before limit

    if ($stapm_value -gt ($stapm_limit - $stapm_hysteresis)) {
        $stapm_time = [ryzen.adj]::get_stapm_time($ry)
        $reduced_stapm_limit = ($stapm_limit - 5) #reduce stapm by 5W
        Write-Host "[STAPM_RESET] stapm_value ($stapm_value) nearing stapm_limit ($stapm_limit), resetting..."
        [void][ryzen.adj]::set_stapm_limit($ry, ($reduced_stapm_limit) * 1000)
        [void][ryzen.adj]::set_stapm_time($ry, 0)
        [Threading.Thread]::Sleep(10) #10ms is usually enough time
        [void][ryzen.adj]::set_stapm_time($ry, $stapm_time)
        [void][ryzen.adj]::set_stapm_limit($ry, $stapm_limit * 1250) # add 25% STAPM limit in case we are at battery saving mode where applied values get reduced by 10% or 20%
    }
}

if(-not $Script:repeatWaitTimeSeconds) { $Script:repeatWaitTimeSeconds = 5 }
$Script:monitorFieldAdjResult = 0; #adjust result will be used for monitoring because SMU may only set 90% and 80% of your value
$Script:monitorFieldAdjTarget = 0;
$powerkey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SYSTEM\ControlSet001\Control\Power\User\PowerSchemes\")
$Script:betterBattery = "961cc777-2547-4f9d-8174-7d86181b8a7a"
$Script:betterPerformance = "00000000-0000-0000-0000-000000000000"
$Script:bestPerformance = "ded574b5-45a0-4f42-8737-46345c09c238"
$Script:acSlider = $powerkey.GetValue("ActiveOverlayACPowerScheme")
$Script:dcSlider = $powerkey.GetValue("ActiveOverlayDCPowerScheme")

$systemPowerStatus = New-Object ryzen.adj+SystemPowerStatus
[void][ryzen.adj]::GetSystemPowerStatus([ref]$systemPowerStatus)
$Script:acLineStatus = $systemPowerStatus.ACLineStatus

testConfiguration

<# Example how to get 560 lines of ptable
$pmTable = [float[]]::new(560)
$tablePtr = [ryzen.adj]::get_table_values($ry);
[System.Runtime.InteropServices.Marshal]::Copy($tablePtr, $pmTable, 0, 560);
#>
createOrDeleteHWINFOSensors

$mtxtArray = @()
if($monitorField){$mtxtArray += "$monitorField changes"}
if($monitorPowerSlider){$mtxtArray += "PowerSlider changes"}
if($monitorACLineStatus){$mtxtArray += "ACLineStatus changes"}
if($mtxtArray.Length){
    $processType = "Monitor " + ($mtxtArray -join " and ")
} else {
    $processType = "Apply Settings"
}
Write-Host "$processType every $Script:repeatWaitTimeSeconds seconds..."
while($true) {
    $doAdjust = !$monitorField -and !$monitorPowerSlider -and !$monitorACLineStatus
    if($monitorField -or $updateHWINFOSensors -or $resetSTAPMUsage) {
        [void][ryzen.adj]::refresh_table($ry)
        #[System.Runtime.InteropServices.Marshal]::Copy($tablePtr, $pmTable, 0, 560);
    }

    if($updateHWINFOSensors){
        updateHWINFOSensors
    }

    if($monitorPowerSlider -and ($Script:acSlider -ne $powerkey.GetValue("ActiveOverlayACPowerScheme") -or $Script:dcSlider -ne $powerkey.GetValue("ActiveOverlayDCPowerScheme"))){
        Write-Host "Power Slider changed"
        $Script:acSlider = $powerkey.GetValue("ActiveOverlayACPowerScheme")
        $Script:dcSlider = $powerkey.GetValue("ActiveOverlayDCPowerScheme")
        $doAdjust = $true
    }
    if($monitorField){
        $monitorValue = getMonitorValue
        if($Script:monitorFieldAdjResult -ne [math]::round($monitorValue, 3, 0)){
            Write-Host "$monitorField value unexpectedly changed from $Script:monitorFieldAdjResult to $monitorValue"
            $doAdjust = $true
        }
    }
    if($monitorACLineStatus){
        [void][ryzen.adj]::GetSystemPowerStatus([ref]$systemPowerStatus)
        if($Script:acLineStatus -ne $systemPowerStatus.ACLineStatus){
            Write-Host "AC Line Status changed"
            $Script:acLineStatus = $systemPowerStatus.ACLineStatus
            $doAdjust = $true
        }
    }

    if($resetSTAPMUsage){
        resetSTAPMIfNeeded
    }

    if($doAdjust){
        [void][ryzen.adj]::GetSystemPowerStatus([ref]$systemPowerStatus)
        $oldWait = $Script:repeatWaitTimeSeconds
        if($systemPowerStatus.ACLineStatus){
            doAdjust_ACmode
        } else {
            doAdjust_BatteryMode
        }
        updateMonitorFieldAdjResult
        if($oldWait -ne $Script:repeatWaitTimeSeconds ) { Write-Host "$processType every $Script:repeatWaitTimeSeconds seconds..." }
    }

    sleep $Script:repeatWaitTimeSeconds
}
