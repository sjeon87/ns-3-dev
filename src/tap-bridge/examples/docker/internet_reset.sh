#!/bin/bash
#
# Undo internet_setup.sh: detach the physical interface from the bridge and ask
# for a fresh DHCP lease on it.
#
# Usage: sudo bash internet_reset.sh <internet device>

set -euo pipefail

if [ -z "${1:-}" ]; then
    echo "Usage: $0 <internet device>" >&2
    exit 1
fi

INTERNET=$1

if ! ip link show "$INTERNET" >/dev/null 2>&1; then
    echo "error: no such interface: $INTERNET" >&2
    exit 1
fi

ip link set dev "$INTERNET" nomaster
ip link set "$INTERNET" down
ip link set "$INTERNET" up

# Distributions ship different DHCP clients, and the ISC dhclient that this
# example originally called is no longer present on current releases. Use
# whichever one is installed, and say so if none is.
if command -v nmcli >/dev/null 2>&1 && nmcli -t device status 2>/dev/null |
    grep -q "^$INTERNET:" && ! nmcli -t device status 2>/dev/null |
    grep -q "^$INTERNET:[^:]*:unmanaged"; then
    nmcli device connect "$INTERNET"
elif command -v dhcpcd >/dev/null 2>&1; then
    dhcpcd -n "$INTERNET"
elif command -v dhclient >/dev/null 2>&1; then
    rm -f /var/lib/dhclient/dhclient.leases
    dhclient "$INTERNET"
elif command -v udhcpc >/dev/null 2>&1; then
    udhcpc -i "$INTERNET"
else
    echo "warning: no DHCP client found, renew the lease on $INTERNET yourself" >&2
fi
