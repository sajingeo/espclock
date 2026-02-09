#!/usr/bin/env python3

import sys
import serial
import serial.tools.list_ports
import time
import argparse

# Colors for terminal output
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
BLUE = '\033[0;34m'
NC = '\033[0m'  # No Color

def find_arduino_port():
    """Find the first Arduino/USB modem port"""
    ports = serial.tools.list_ports.comports()
    
    # First, try to find usbmodem ports (typical for Arduino)
    for port in ports:
        if 'usbmodem' in port.device:
            return port.device
    
    # Then try other USB serial ports
    for port in ports:
        if 'usbserial' in port.device or 'USB' in port.device:
            return port.device
    
    return None

def send_command(command):
    """Send command to Weather Puck"""
    port_name = find_arduino_port()
    
    if not port_name:
        print(f"{RED}Error: No USB serial port found{NC}")
        print("Available ports:")
        for port in serial.tools.list_ports.comports():
            print(f"  {port.device} - {port.description}")
        return False
    
    try:
        # Open serial port
        with serial.Serial(port_name, 115200, timeout=2) as ser:
            time.sleep(0.1)  # Wait for port to be ready
            
            # Clear any pending data
            ser.reset_input_buffer()
            
            # Send command
            ser.write(f"{command}\n".encode())
            print(f"{GREEN}✓ Sent to {port_name}:{NC} {command}")
            
            # Try to read response
            print(f"{BLUE}Response:{NC}")
            time.sleep(0.2)  # Give device time to respond
            
            # Read available lines
            response_count = 0
            while ser.in_waiting > 0 and response_count < 5:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"  {line}")
                    response_count += 1
            
            if response_count == 0:
                print("  (No response - check device display)")
            
            return True
            
    except serial.SerialException as e:
        if "busy" in str(e).lower() or "access" in str(e).lower():
            print(f"{YELLOW}⚠️  Port {port_name} is busy{NC}")
            print(f"    Close Arduino Serial Monitor and try again")
            print(f"    Command '{command}' may still have been sent")
        else:
            print(f"{RED}Error: {e}{NC}")
        return False

def parse_time(time_str):
    """Parse HH:MM format"""
    try:
        parts = time_str.split(':')
        if len(parts) != 2:
            return None
        hours = int(parts[0])
        minutes = int(parts[1])
        if minutes > 59:
            return None
        return hours, minutes
    except:
        return None

def main():
    parser = argparse.ArgumentParser(description='Weather Puck Timer Control')
    parser.add_argument('-t', '--timer', help='Start timer HH:MM (e.g., 1:30)')
    parser.add_argument('-s', '--stop', action='store_true', help='Stop timer')
    parser.add_argument('-r', '--status', action='store_true', help='Get timer status')
    parser.add_argument('-l', '--list', action='store_true', help='List available ports')
    parser.add_argument('-p', '--port', help='Use specific port (e.g., /dev/tty.usbmodem2134201)')
    
    args = parser.parse_args()
    
    # Override port if specified
    if args.port:
        global find_arduino_port
        find_arduino_port = lambda: args.port
    
    if args.list:
        print("Available serial ports:")
        for port in serial.tools.list_ports.comports():
            print(f"  {port.device} - {port.description}")
        sys.exit(0)
    
    if args.timer:
        time_parts = parse_time(args.timer)
        if not time_parts:
            print(f"{RED}Error: Invalid time format. Use HH:MM{NC}")
            sys.exit(1)
        
        hours, minutes = time_parts
        print(f"{YELLOW}⏱  Starting timer: {hours} hour(s) {minutes} minute(s){NC}")
        send_command(f"start_timer {args.timer}")
        
    elif args.stop:
        print(f"{YELLOW}⏹  Stopping timer...{NC}")
        send_command("stop_timer")
        
    elif args.status:
        print(f"{YELLOW}📊 Timer status:{NC}")
        send_command("status")
        
    else:
        parser.print_help()

if __name__ == "__main__":
    main()