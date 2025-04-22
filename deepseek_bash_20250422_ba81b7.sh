#!/bin/bash

# Paths
USBIP_DEVICES="/etc/usbip/devices.conf"
LOG_FILE="/var/log/usbip-manager.log"

# Load required kernel modules
modprobe usbip-core
modprobe usbip-host
modprobe vhci-hcd

# Function to bind all devices listed in $USBIP_DEVICES
bind_devices() {
  echo "Starting USB/IP binding..." >> "$LOG_FILE"
  
  # Read device list
  while read -r bus_id; do
    if [[ -z "$bus_id" || "$bus_id" == \#* ]]; then
      continue  # Skip empty lines and comments
    fi
    
    echo "Binding device $bus_id..." >> "$LOG_FILE"
    usbip bind -b "$bus_id" >> "$LOG_FILE" 2>&1
    
    if [ $? -eq 0 ]; then
      echo "Successfully bound $bus_id" >> "$LOG_FILE"
    else
      echo "Failed to bind $bus_id" >> "$LOG_FILE"
    fi
  done < "$USBIP_DEVICES"
}

# Function to unbind all devices
unbind_devices() {
  echo "Unbinding all devices..." >> "$LOG_FILE"
  for port in $(usbip port | grep -oP 'Port \K[0-9]+'); do
    usbip unbind -p "$port" >> "$LOG_FILE" 2>&1
  done
}

# Main logic
case "$1" in
  start)
    bind_devices
    ;;
  stop)
    unbind_devices
    ;;
  *)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;
esac