#!/bin/bash

# OpenOCD 连接测试脚本
# 此脚本测试 OpenOCD 连接并提供诊断信息

set -euo pipefail

readonly TIMEOUT_CONFIG=10
readonly TIMEOUT_CONNECT=10
CONFIG_FILE=""
readonly TEMP_DIR=$(mktemp -d)
readonly LOG_FILE="${TEMP_DIR}/openocd_test.log"

# 颜色定义
readonly GREEN='\033[0;32m'
readonly RED='\033[0;31m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

# 显示使用说明
show_help() {
    cat << 'EOF'
OpenOCD 连接测试脚本

用法: ./validate_openocd.sh [选项]

选项:
  -c <配置文件>   指定 OpenOCD 配置文件路径（必需）
  -h              显示此帮助信息

示例:
  ./validate_openocd.sh -c openocd.cfg
  ./validate_openocd.sh -c boards/nucleo_g431rb/openocd.cfg
  ./validate_openocd.sh -h

说明:
  此脚本测试 OpenOCD 连接并提供诊断信息。需要指定配置文件路径，
  因为项目中可能存在多个 openocd.cfg 文件。
EOF
}

# 解析命令行参数
parse_arguments() {
    while getopts ":c:h" opt; do
        case ${opt} in
            c )
                CONFIG_FILE="${OPTARG}"
                ;;
            h )
                show_help
                exit 0
                ;;
            \? )
                echo -e "${RED}错误: 无效选项 -${OPTARG}${NC}" >&2
                echo "使用 -h 查看帮助信息"
                exit 1
                ;;
            : )
                echo -e "${RED}错误: -${OPTARG} 选项需要参数${NC}" >&2
                echo "使用 -h 查看帮助信息"
                exit 1
                ;;
        esac
    done
    
    # 检查是否提供了配置文件
    if [[ -z "${CONFIG_FILE}" ]]; then
        echo -e "${RED}错误: 未指定配置文件${NC}" >&2
        echo "请使用 -c 选项指定 OpenOCD 配置文件路径"
        echo ""
        show_help
        exit 1
    fi
}

cleanup() {
    rm -rf "${TEMP_DIR}"
}
trap cleanup EXIT

check_config_exists() {
    if [[ ! -f "${CONFIG_FILE}" ]]; then
        echo -e "${RED}✗ 错误: 配置文件 ${CONFIG_FILE} 不存在${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓${NC} 配置文件 ${CONFIG_FILE} 存在"
}

# 检查用户权限
check_wsl_environment() {
    echo -e "${BLUE}1.${NC} 正在检查运行环境..."
    if grep -qEi "(Microsoft|WSL)" /proc/version &>/dev/null; then
        echo -e "   ${GREEN}✓${NC} 检测到 WSL2 环境"
        
        if [[ -f "$(dirname "$0")/stlink_wsl_usbip_tools/stlink_wsl_usbip_validator.sh" ]]; then
            echo ""
            echo -e "   正在验证 USB/IP 环境..."
            if "$(dirname "$0")/stlink_wsl_usbip_tools/stlink_wsl_usbip_validator.sh" --quiet; then
                echo -e "   ${GREEN}✓${NC} USB/IP 环境验证通过，ST-Link 设备已正确连接"
            else
                echo -e "   ${YELLOW}⚠${NC}  USB/IP 环境验证失败"
                echo -e "   ${BLUE}提示${NC}: 在WSL中使用OpenOCD前，请先确保ST-Link设备已正确连接"
                echo -e "   ${BLUE}提示${NC}: 可以手动运行以下命令查看详细信息:"
                echo -e "   ${BLUE}./stlink_wsl_usbip_tools/stlink_wsl_usbip_validator.sh${NC}"
            fi
        else
            echo ""
            echo -e "   ${YELLOW}⚠${NC}  USB/IP 验证工具不可用"
            echo -e "   ${BLUE}提示${NC}: 在WSL中使用OpenOCD前，请先确保ST-Link设备已正确连接"
            echo -e "   使用 lsusb 命令检查设备是否可见"
        fi
    else
        echo -e "   ${GREEN}✓${NC} 检测到原生Linux环境"
        if command -v lsusb &>/dev/null; then
            local stlink_devices
            stlink_devices=$(lsusb | grep -i "st-link\|stmicroelectronics" || true)
            if [[ -n "$stlink_devices" ]]; then
                local device_count
                device_count=$(echo "$stlink_devices" | wc -l)
                echo -e "   ${GREEN}✓${NC} 找到 ${device_count} 个 ST-Link 设备"
            else
                echo -e "   ${YELLOW}⚠${NC}  未找到 ST-Link 设备，请检查USB连接"
            fi
        else
            echo -e "   ${YELLOW}⚠${NC}  lsusb 命令不可用，无法检查设备"
        fi
    fi
    echo ""
}

check_user_permissions() {
    echo -e "${BLUE}2.${NC} 正在检查用户权限..."
    if [[ "$EUID" -eq 0 ]]; then
        echo -e "   ${GREEN}✓${NC} 以 root 用户运行"
    else
        echo -e "   ${GREEN}✓${NC} 以普通用户运行（组: $(id -Gn)）"
    fi
    echo ""
}

# 检查 OpenOCD 安装
check_openocd_installation() {
    echo -e "${BLUE}3.${NC} 正在检查 OpenOCD 安装..."
    if command -v openocd &>/dev/null; then
        local openocd_version
        openocd_version=$(openocd --version 2>&1 | head -n1) || true
        if [[ -n "$openocd_version" ]]; then
            echo -e "   ${GREEN}✓${NC} 找到 OpenOCD: $openocd_version"
        else
            echo -e "   ${GREEN}✓${NC} 找到 OpenOCD: 版本信息不可用"
        fi
    else
        echo -e "   ${RED}✗${NC} 在 PATH 中未找到 OpenOCD"
        echo "   安装命令: sudo apt-get install openocd"
    fi
    echo ""
}

# 测试 OpenOCD 配置
test_openocd_config() {
    echo -e "${BLUE}4.${NC} 正在测试 OpenOCD 配置..."
    
    local exit_code=0
    timeout "${TIMEOUT_CONFIG}s" openocd -f "${CONFIG_FILE}" -c "init; echo \"Config OK\"; exit" >"${LOG_FILE}" 2>&1 || exit_code=$?
    
    if [[ $exit_code -eq 0 ]] || [[ $exit_code -eq 124 ]]; then
        if [[ $exit_code -eq 124 ]]; then
            echo -e "   ${YELLOW}⚠${NC} 配置测试超时，但语法可能正常"
        else
            echo -e "   ${GREEN}✓${NC} OpenOCD 配置语法正常"
        fi
        
        if grep -qi "error\|fatal" "${LOG_FILE}"; then
            echo -e "   ${RED}✗${NC} 但发现错误:"
            grep -i "error\|fatal" "${LOG_FILE}" | sed 's/^/   /'
        fi
        
        if grep -qi "warning" "${LOG_FILE}"; then
            echo -e "   ${YELLOW}⚠${NC} 警告信息:"
            grep -i "warning" "${LOG_FILE}" | sed 's/^/   /'
        fi
    else
        echo -e "   ${RED}✗${NC} OpenOCD 配置有错误:"
        cat "${LOG_FILE}" | sed 's/^/   /'
    fi
    echo ""
}

# 检查 udev 规则（仅在非 WSL 环境中检查）
check_udev_rules() {
    if grep -qEi "(Microsoft|WSL)" /proc/version &>/dev/null; then
        echo -e "${BLUE}5.${NC} 正在检查 udev 规则..."
        echo -e "   ${GREEN}✓${NC} WSL 环境 - 跳过 udev 规则检查（WSL 不使用 udev）"
        echo ""
        return 0
    fi
    
    echo -e "${BLUE}5.${NC} 正在检查 udev 规则..."
    local udev_rule_found=false
    local rule_files=(
        "/etc/udev/rules.d/49-stlinkv1.rules"
        "/etc/udev/rules.d/49-stlinkv2.rules"
        "/etc/udev/rules.d/49-stlinkv2-1.rules"
        "/etc/udev/rules.d/49-stlinkv3.rules"
    )
    
    for rule_file in "${rule_files[@]}"; do
        if [[ -f "$rule_file" ]]; then
            echo -e "   ${GREEN}✓${NC} 找到 udev 规则: $rule_file"
            udev_rule_found=true
        fi
    done
    
    if [[ "$udev_rule_found" == "false" ]]; then
        echo -e "   ${YELLOW}⚠${NC} 未找到 ST-Link udev 规则"
        echo "   您可能需要为 USB 访问创建 udev 规则"
        echo "   运行: sudo cat > /etc/udev/rules.d/49-stlinkv3.rules << 'EOF'"
        echo '   SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374*", MODE:="0666", GROUP="plugdev"'
        echo "   EOF"
        echo "   然后: sudo udevadm control --reload-rules && sudo udevadm trigger"
    fi
    echo ""
}

# 测试与目标设备的连接
test_target_connection() {
    echo -e "${BLUE}6.${NC} 正在测试与目标设备的连接（${TIMEOUT_CONNECT} 秒）..."
    echo "   请确保开发板已连接并通电。"
    echo ""
    
    local exit_code=0
    timeout "${TIMEOUT_CONNECT}s" openocd -c "gdb_port 0; telnet_port 0" -f "${CONFIG_FILE}" -c "init; exit" >"${LOG_FILE}" 2>&1 || exit_code=$?
    
    # 显示关键信息
    grep -E "(STLINK|Target voltage|Cortex-M|breakpoints|watchpoints)" "${LOG_FILE}" 2>/dev/null | sed 's/^/   /' || true
    
    echo ""
    if grep -q "Cortex-M" "${LOG_FILE}" 2>/dev/null; then
        echo -e "   ${GREEN}✓${NC} 成功检测到 Cortex-M4 处理器"
    fi
    if grep -q "STLINK" "${LOG_FILE}" 2>/dev/null; then
        echo -e "   ${GREEN}✓${NC} ST-Link 调试器连接正常"
    fi
    if grep -q "breakpoints" "${LOG_FILE}" 2>/dev/null; then
        echo -e "   ${GREEN}✓${NC} 目标设备已准备好调试"
    fi
    
    return $exit_code
}

# 处理测试结果
handle_test_result() {
    local exit_code=$1
    
    echo ""
    echo "==================================="
    
    if [[ $exit_code -eq 124 ]]; then
        echo -e "${YELLOW}结果: 超时 - 开发板无响应${NC}"
        echo ""
        echo "故障排除步骤:"
        echo "1. 检查 USB 连接（尝试不同的端口/线缆）"
        echo "2. 检查开发板电源（应该能看到 LED）"
        echo "3. 检查 ST-Link 固件（使用 STM32CubeProgrammer 更新）"
        echo "4. 如果权限被拒绝，尝试用 sudo 运行"
        echo "5. 检查 dmesg 中的 USB 错误: dmesg | tail -20"
    elif [[ $exit_code -eq 0 ]]; then
        echo -e "${GREEN}结果: 成功! 开发板已连接并被识别。${NC}"
        echo ""
        echo "您现在可以使用:"
        echo "- 烧录: openocd -f openocd.cfg -c 'program build/CubeMot.bin 0x08000000 verify reset exit'"
        echo "- 调试: openocd -f openocd.cfg"
    else
        echo -e "${RED}结果: 失败，退出码 ${exit_code}${NC}"
        echo ""
        echo "请查看上面的错误信息了解详情"
        
        if [[ -s "${LOG_FILE}" ]]; then
            echo ""
            echo -e "${RED}详细错误日志:${NC}"
            cat "${LOG_FILE}" | sed 's/^/   /'
        fi
    fi
    
    echo "==================================="
}

# 主执行流程
main() {
    # 解析命令行参数
    parse_arguments "$@"
    
    echo "==================================="
    echo "OpenOCD 连接测试"
    echo "配置文件: ${CONFIG_FILE}"
    echo "==================================="
    echo ""
    
    check_config_exists
    echo ""
    
    check_wsl_environment
    check_user_permissions
    check_openocd_installation
    test_openocd_config
    check_udev_rules
    
    test_target_connection
    handle_test_result $?
}

main "$@"
