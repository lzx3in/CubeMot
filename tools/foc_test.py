#!/usr/bin/env python3
"""
FOC 电机测试脚本

通过串口发送 CMD_TEST 命令控制电机

命令格式: [0xAA 0x55] [CMD_ID] [LEN] [PAYLOAD...] [CRC8]
CMD_TEST = 0x10
  test_id=0: 启动 ISR, Id=param (对齐测试)
  test_id=1: 停止 ISR
  test_id=2: 设置 Iq=param (扭矩测试)
"""

import serial
import struct
import time
import sys

def crc8(data):
    """CRC8 校验 (多项式 0x07)"""
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc

def build_frame(cmd_id, payload=b''):
    """构建命令帧"""
    frame = bytes([0xAA, 0x55, cmd_id, len(payload)]) + payload
    frame += bytes([crc8(frame[2:])])
    return frame

def cmd_test_start_id(id_amps):
    """启动 ISR, 设置 Id 电流 (对齐测试)"""
    payload = struct.pack('<Bf', 0, id_amps)  # test_id=0, Id=param
    return build_frame(0x10, payload)

def cmd_test_stop():
    """停止 ISR"""
    payload = struct.pack('<Bf', 1, 0.0)  # test_id=1
    return build_frame(0x10, payload)

def cmd_test_set_iq(iq_amps):
    """设置 Iq 电流 (扭矩测试)"""
    payload = struct.pack('<Bf', 2, iq_amps)  # test_id=2, Iq=param
    return build_frame(0x10, payload)

def main():
    if len(sys.argv) < 2:
        print("用法:")
        print("  python foc_test.py start <Id_amps>  - 启动 ISR (对齐测试)")
        print("  python foc_test.py stop             - 停止 ISR")
        print("  python foc_test.py iq <Iq_amps>     - 设置 Iq (扭矩测试)")
        print("\n示例:")
        print("  python foc_test.py start 0.5   # 启动, Id=0.5A")
        print("  python foc_test.py iq 0.3      # 设置 Iq=0.3A")
        print("  python foc_test.py stop        # 停止")
        sys.exit(1)

    # 默认串口 (根据实际连接修改)
    port = '/dev/ttyACM0'  # Nucleo 板载 ST-Link VCP
    baud = 115200

    try:
        ser = serial.Serial(port, baud, timeout=1)
        print(f"已连接 {port} @ {baud}")
    except serial.SerialException as e:
        print(f"串口打开失败: {e}")
        print("请检查:")
        print("  1. Nucleo 板是否连接")
        print("  2. 串口设备是否正确 (ls /dev/ttyACM*)")
        sys.exit(1)

    cmd = sys.argv[1]

    if cmd == 'start':
        amps = float(sys.argv[2]) if len(sys.argv) > 2 else 0.5
        frame = cmd_test_start_id(amps)
        print(f"启动 ISR, Id={amps}A")
        ser.write(frame)

    elif cmd == 'stop':
        frame = cmd_test_stop()
        print("停止 ISR")
        ser.write(frame)

    elif cmd == 'iq':
        amps = float(sys.argv[2]) if len(sys.argv) > 2 else 0.3
        frame = cmd_test_set_iq(amps)
        print(f"设置 Iq={amps}A")
        ser.write(frame)

    else:
        print(f"未知命令: {cmd}")

    # 读取响应 (可选)
    time.sleep(0.1)
    if ser.in_waiting > 0:
        resp = ser.read(ser.in_waiting)
        print(f"响应: {resp.hex()}")

    ser.close()

if __name__ == '__main__':
    main()
