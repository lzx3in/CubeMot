#!/bin/bash

# ST-Link USB Device Connectivity Validator for WSL
# This script validates the WSL environment and USB/IP prerequisites for ST-Link debugging
#
# USAGE:
#   Standalone: Run this script directly to validate WSL environment
#   With Manager: Run stlink-wsl-usbip-manager.ps1 in Windows PowerShell (Admin)
#                 to automatically attach/detach ST-Link devices
#   From other scripts: Call validate_stlink_environment() function
#
# OPTIONS:
#   -q, --quiet    Quiet mode, minimal output
#   -j, --json     JSON output for script consumption
#   -h, --help     Show this help message

# Global variables
QUIET_MODE=0
JSON_OUTPUT=0
SCRIPT_RESULT=0

# Print message based on quiet mode
print_info() {
    if [ $QUIET_MODE -eq 0 ]; then
        echo "$@"
    fi
}

# Print error message (always shown)
print_error() {
    echo "$@" >&2
}

# Show usage
show_help() {
    cat << EOF
ST-Link WSL USB/IP Environment Validator

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -q, --quiet    Quiet mode, minimal output (for script calling)
    -j, --json     JSON output format
    -h, --help     Show this help message

EXAMPLES:
    # Interactive use (default)
    $0

    # Quiet validation from another script
    $0 --quiet

    # Get JSON results
    $0 --json

    # Call from another script
    source $0
    validate_stlink_environment
    if [ $? -eq 0 ]; then
        echo "Environment is ready"
    fi
EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -q|--quiet)
                QUIET_MODE=1
                shift
                ;;
            -j|--json)
                JSON_OUTPUT=1
                QUIET_MODE=1
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Core validation function for use by other scripts
# Returns: 0 on success, 1 on failure
validate_stlink_environment() {
    local result=0
    local errors=""
    local checks=""
    
    # Check 1: Verify WSL environment
    if [ $JSON_OUTPUT -eq 0 ]; then
        print_info "1. Checking WSL Environment..."
    fi
    
    local wsl_check=0
    if grep -qEi "(Microsoft|WSL)" /proc/version &> /dev/null; then
        print_info "   ✓ Running in WSL2 environment"
        checks="${checks}\"wsl_environment\": true,"
    else
        print_error "   ⚠ Not running in WSL (this script requires WSL2)"
        checks="${checks}\"wsl_environment\": false,"
        errors="${errors}\"Not running in WSL2 environment."
        result=1
    fi
    
    # Check 2: Check usbip client availability
    if [ $JSON_OUTPUT -eq 0 ]; then
        print_info ""
        print_info "2. Checking USB/IP Client..."
    fi
    
    local usbip_check=0
    if command -v usbip &> /dev/null; then
        print_info "   ✓ usbip client is available"
        checks="${checks}\"usbip_client\": true,"
    else
        print_error "   ✗ usbip client not found"
        print_error "   Install with: sudo apt install -y usbip"
        checks="${checks}\"usbip_client\": false,"
        errors="${errors}\"usbip client not found. Install usbip package with: sudo apt install -y usbip"
        result=1
        usbip_check=1
    fi
    
    # Check 3: Check usbip_host kernel module
    if [ $JSON_OUTPUT -eq 0 ]; then
        print_info ""
        print_info "3. Checking USB/IP Kernel Modules..."
    fi
    
    local module_loaded=0
    if lsmod | grep -q "usbip_host"; then
        print_info "   ✓ usbip_host kernel module is loaded"
        checks="${checks}\"usbip_host_module\": true,"
        module_loaded=1
    else
        print_info "   ⚠ usbip_host module not loaded, attempting to load..."
        sudo modprobe usbip_host
        if [ $? -eq 0 ]; then
            print_info "   ✓ Module loaded successfully"
            checks="${checks}\"usbip_host_module\": true,"
            module_loaded=1
        else
            print_error "   ✗ Failed to load usbip_host module"
            print_error "   You may need to install linux-modules-extra: sudo apt-get install linux-modules-extra-\$(uname -r)"
            checks="${checks}\"usbip_host_module\": false,"
            errors="${errors}\"Failed to load usbip_host module."
            result=1
        fi
    fi
    
    # Check 4: Enumerate USB devices in WSL
    if [ $JSON_OUTPUT -eq 0 ]; then
        print_info ""
        print_info "4. Enumerating USB Devices in WSL:"
    fi
    
    local usb_devices=""
    local stlink_found=0
    if command -v lsusb &> /dev/null; then
        USB_COUNT=$(lsusb 2>/dev/null | wc -l)
        if [ $USB_COUNT -gt 0 ]; then
            usb_devices=$(lsusb 2>/dev/null)
            if [ $JSON_OUTPUT -eq 0 ]; then
                print_info "   $usb_devices"
            fi
            
            # Check for ST-Link devices
            if echo "$usb_devices" | grep -qi "st-link"; then
                stlink_found=1
            fi
        else
            print_info "   No USB devices found in WSL"
        fi
        checks="${checks}\"lsusb_available\": true,"
    else
        print_info "   ⚠ lsusb command not available"
        print_info "   Install with: sudo apt-get install usbutils"
        checks="${checks}\"lsusb_available\": false,"
        errors="${errors}\"lsusb command not available."
        result=1
    fi
    
    checks="${checks}\"stlink_device_found\": $stlink_found"
    
    # JSON output
    if [ $JSON_OUTPUT -eq 1 ]; then
        local json_output="{"
        json_output="${json_output}\"success\": $([ $result -eq 0 ] && echo "true" || echo "false"),"
        json_output="${json_output}\"checks\": {${checks}},"
        if [ -n "$errors" ]; then
            # Remove trailing comma from errors and format as JSON array
            errors="${errors%,}"
            json_output="${json_output}\"errors\": ["
            local first=1
            IFS='"' read -ra ADDR <<< "$errors"
            for i in "${ADDR[@]}"; do
                if [ -n "$i" ] && [ "$i" != " " ]; then
                    if [ $first -eq 1 ]; then
                        first=0
                    else
                        json_output="${json_output},"
                    fi
                    json_output="${json_output}\"$i\""
                fi
            done
            json_output="${json_output}]"
        else
            json_output="${json_output}\"errors\": []"
        fi
        json_output="${json_output}}"
        echo "$json_output"
    fi
    
    return $result
}

# Interactive mode with user-friendly output
run_interactive() {
    print_info "==================================="
    print_info "ST-Link WSL USB/IP Environment Validator"
    print_info "==================================="
    print_info ""
    print_info "Note: For automated device management, use stlink-wsl-usbip-manager.ps1"
    print_info "      in Windows PowerShell (Administrator)"
    print_info ""
    
    validate_stlink_environment
    local result=$?
    
    print_info ""
    print_info "==================================="
    print_info "Next Steps:"
    print_info "==================================="
    print_info ""
    print_info "Option 1 - Automated (Recommended):"
    print_info "  Run in Windows PowerShell (as Administrator):"
    print_info "  stlink-wsl-usbip-manager.ps1"
    print_info ""
    print_info "Option 2 - Manual:"
    print_info "  Run in Windows PowerShell/CMD (as Administrator):"
    print_info "    usbipd list"
    print_info ""
    print_info "  Locate your ST-Link device (typically Vendor ID: 0483, Product ID: 374b for ST-Link V3)"
    print_info ""
    print_info "  Attach it to WSL with:"
    print_info "    usbipd attach --wsl --busid=<BUS-ID>"
    print_info ""
    print_info "  Example: usbipd attach --wsl --busid=2-1"
    print_info ""
    print_info "After attaching, re-run this script or use 'lsusb' to verify connectivity."
    print_info "==================================="
    
    return $result
}

# Function to get validation result without side effects
# Usage: result=$(check_stlink_environment)
check_stlink_environment() {
    local quiet=$QUIET_MODE
    local json=$JSON_OUTPUT
    QUIET_MODE=1
    JSON_OUTPUT=0
    validate_stlink_environment
    local result=$?
    QUIET_MODE=$quiet
    JSON_OUTPUT=$json
    return $result
}

# Main execution block
# Detect if script is being sourced or executed
if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    # Script is being sourced
    # Export functions for use by other scripts
    export -f validate_stlink_environment
    export -f check_stlink_environment
    export -f print_info
    export -f print_error
else
    # Script is being executed directly
    parse_args "$@"
    
    if [ $JSON_OUTPUT -eq 1 ]; then
        validate_stlink_environment
        exit $?
    else
        run_interactive
        exit $?
    fi
fi
