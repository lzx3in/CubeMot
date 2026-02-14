# ST-Link USB/IP Device Manager for WSL
# Execute this script in Windows PowerShell with Administrator privileges to manage ST-Link device attachment
#
# USAGE:
#   Standalone: Run this script directly in PowerShell (Admin) to attach/detach devices
#   With Validator: After attaching, run stlink-wsl-usbip-validator.sh in WSL
#                   to verify connectivity and environment setup

Write-Host "===================================" -ForegroundColor Cyan
Write-Host "ST-Link USB/IP WSL Device Manager" -ForegroundColor Cyan
Write-Host "===================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Note: After device attachment, run stlink-wsl-usbip-validator.sh in WSL"
Write-Host "      to verify connectivity and environment setup" -ForegroundColor Yellow
Write-Host ""

# Verify usbipd installation
if (-not (Get-Command usbipd -ErrorAction SilentlyContinue)) {
    Write-Host "✗ usbipd command not found!" -ForegroundColor Red
    Write-Host "Please install usbipd-win:" -ForegroundColor Yellow
    Write-Host "  winget install --interactive --exact dorssel.usbipd-win" -ForegroundColor White
    exit 1
}

Write-Host "✓ usbipd is available" -ForegroundColor Green
Write-Host ""

# Enumerate ST-Link devices
Write-Host "Scanning for ST-Link debug probes..." -ForegroundColor Cyan
$devices = usbipd list | Select-String "ST-Link" -Context 0,0

if (-not $devices) {
    Write-Host "✗ No ST-Link devices detected!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Troubleshooting steps:" -ForegroundColor Yellow
    Write-Host "1. Verify USB cable connection" -ForegroundColor White
    Write-Host "2. Try a different USB port" -ForegroundColor White
    Write-Host "3. Ensure ST-Link firmware is up to date" -ForegroundColor White
    Write-Host "4. Execute 'usbipd list' to view all USB devices" -ForegroundColor White
    exit 1
}

# Process device selection
if ($devices.Count -eq 1) {
    $deviceLine = $devices[0].ToString()
    $busId = $deviceLine -replace '.*?(\d+-\d+).*', '$1'
    
    Write-Host "Detected 1 ST-Link device:" -ForegroundColor Green
    Write-Host $deviceLine.Trim() -ForegroundColor White
    Write-Host ""
} else {
    Write-Host "Detected $($devices.Count) ST-Link devices:" -ForegroundColor Green
    Write-Host ""
    
    for ($i = 0; $i -lt $devices.Count; $i++) {
        $deviceInfo = $devices[$i].ToString().Trim()
        Write-Host "[$($i+1)] $deviceInfo" -ForegroundColor White
    }
    Write-Host ""
    
    $selection = 0
    while ($selection -lt 1 -or $selection -gt $devices.Count) {
        try {
            $selection = [int](Read-Host "Select device number [1-$($devices.Count)]")
            if ($selection -lt 1 -or $selection -gt $devices.Count) {
                Write-Host "Invalid selection. Enter a number between 1 and $($devices.Count)." -ForegroundColor Red
            }
        } catch {
            Write-Host "Invalid input. Please enter a numeric value." -ForegroundColor Red
            $selection = 0
        }
    }
    
    $deviceLine = $devices[$selection-1].ToString()
    $busId = $deviceLine -replace '.*?(\d+-\d+).*', '$1'
    
    Write-Host ""
    Write-Host "Selected device:" -ForegroundColor Cyan
    Write-Host $deviceLine.Trim() -ForegroundColor White
    Write-Host ""
}

# Determine current attachment state
$isAttached = $deviceLine -match "Attached"

# Display current status and prompt for action
Write-Host "Current attachment status:" -ForegroundColor Cyan
if ($isAttached) {
    Write-Host "✓ Device is ATTACHED to WSL" -ForegroundColor Green
} else {
    Write-Host "✓ Device is DETACHED from WSL" -ForegroundColor Yellow
}
Write-Host ""

$action = Read-Host "Select operation: [A]ttach, [D]etach, or [C]ancel"

switch ($action.ToUpper()) {
    "A" {
        # Attach operation
        if ($isAttached) {
            Write-Host ""
            Write-Host "✓ Device is already attached to WSL" -ForegroundColor Green
            Write-Host "No operation required." -ForegroundColor Cyan
        } else {
            Write-Host ""
            Write-Host "Attaching ST-Link to WSL..." -ForegroundColor Cyan
            usbipd attach --wsl --busid=$busId
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host ""
                Write-Host "✓ Successfully attached to WSL!" -ForegroundColor Green
                Write-Host ""
                Write-Host "Verification steps in WSL:" -ForegroundColor Cyan
                Write-Host "1. Execute: lsusb" -ForegroundColor White
                Write-Host "2. Verify: STMicroelectronics ST-Link" -ForegroundColor White
                Write-Host ""
                Write-Host "Option A - Quick verification:" -ForegroundColor Cyan
                Write-Host "   Run: lsusb | grep ST-Link" -ForegroundColor White
                Write-Host ""
                Write-Host "Option B - Full validation:" -ForegroundColor Cyan
                Write-Host "   Run: stlink-wsl-usbip-validator.sh" -ForegroundColor White
                Write-Host "   (Validates environment, drivers, and connectivity)" -ForegroundColor White
            } else {
                Write-Host ""
                Write-Host "✗ Failed to attach device" -ForegroundColor Red
                exit 1
            }
        }
    }
    "D" {
        # Detach operation
        if (-not $isAttached) {
            Write-Host ""
            Write-Host "✓ Device is already detached" -ForegroundColor Yellow
            Write-Host "No operation required." -ForegroundColor Cyan
        } else {
            Write-Host ""
            Write-Host "Detaching ST-Link from WSL..." -ForegroundColor Cyan
            usbipd detach --busid=$busId
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host ""
                Write-Host "✓ Successfully detached from WSL!" -ForegroundColor Green
            } else {
                Write-Host ""
                Write-Host "✗ Failed to detach device" -ForegroundColor Red
                exit 1
            }
        }
    }
    "C" {
        # Cancel operation
        Write-Host ""
        Write-Host "Operation cancelled." -ForegroundColor Yellow
        exit 0
    }
    default {
        Write-Host ""
        Write-Host "✗ Invalid selection" -ForegroundColor Red
        exit 1
    }
}

Write-Host ""
Write-Host "===================================" -ForegroundColor Cyan
Write-Host "Operation completed" -ForegroundColor Cyan
Write-Host "===================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Troubleshooting:" -ForegroundColor Yellow
Write-Host "  If you encounter issues after attachment:" -ForegroundColor White
Write-Host "  1. Run stlink-wsl-usbip-validator.sh in WSL to check environment" -ForegroundColor White
Write-Host "  2. Ensure all required packages are installed in WSL" -ForegroundColor White
Write-Host "  3. Try detaching and re-attaching the device" -ForegroundColor White
Write-Host "===================================" -ForegroundColor Cyan
