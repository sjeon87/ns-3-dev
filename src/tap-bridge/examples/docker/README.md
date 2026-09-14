# Emulating a real network with Docker and ns-3

These scripts build a Docker image and set up the bridged networking needed to
run an ns-3 emulation between two containers, as a drop-in replacement for the
LXC containers used by `src/tap-bridge/examples/tap-csma-virtual-machine.cc`.
Because the containers can use the host X display, real applications such as
the Chromium browser can run over the emulated network.

The resulting topology is:

```text
+-----------+                                        +-----------+
| container |                                        | container |
|   left    |                                        |   right   |
|  (eth0)   |                                        |  (eth0)   |
+-----+-----+                                        +-----+-----+
      |                                                    |
 +----+----+                                          +----+----+
 | br-left |                                          | br-right|
 +----+----+                                          +----+----+
      |                                                    |
+-----+----+        +--------------------------+      +----+-----+
| tap-left +--------+  ns-3 CSMA (tap-bridge)  +------+ tap-right|
+----------+        +--------------------------+      +----------+
```

## Prerequisites

A Linux host with Docker, `iproute2` and `util-linux` (for `nsenter`), all of
which are present on any current distribution. For the GUI part, the host also
needs `xhost`, from `x11-xserver-utils` on Debian and Ubuntu or `xorg-xhost` on
Arch Linux.

Podman works as a drop-in for every command these scripts use. Set
`CONTAINER_RUNTIME=podman` to select it. With Podman the containers have to be
rootful, because `setup.sh` runs as root and a root `podman inspect` does not
see the containers a rootless `podman run` created.

Only the steps that touch the host network namespace need root: creating the
bridges and TAP devices, and handing a physical interface to a bridge. Building
the image, entering a container and running ns-3 itself do not.

The scripts use `ip` throughout, matching `virtual-network-setup.sh` in the
parent directory, so the deprecated `bridge-utils` (`brctl`) and
`uml-utilities` (`tunctl`) packages are not needed.

## Steps

Run the scripts from this directory. `setup.sh`, `destroy.sh`,
`internet_setup.sh` and `internet_reset.sh` need root; the remaining steps do
not.

1. Build the container image. The scripts expect it to be tagged `ns3exp:latest`;
   set `NS3_DOCKER_IMAGE` to use a different tag.

   ```shell
   docker build -t ns3exp:latest .
   ```

2. Configure ns-3 and copy the example into `scratch/`.

   ```shell
   cp ../tap-csma-virtual-machine.cc ../../../../scratch/
   cd ../../../.. && ./ns3 configure -d release --enable-sudo && ./ns3 build
   ```

   `--enable-sudo` makes `ns3` chown the built programs to root and set their
   setuid bit, prompting for a password once, so that the simulation can open
   the TAP devices later without being launched under `sudo`.

3. Create the containers, bridges and TAP devices.

   ```shell
   sudo bash setup.sh
   ```

   With Podman, pass the runtime through `sudo`:

   ```shell
   sudo CONTAINER_RUNTIME=podman bash setup.sh
   ```

4. Give the `left` container access to the internet, assuming the internet port
   is `eno1`. The host gives up its own connectivity on that interface until
   step 9.

   ```shell
   sudo bash internet_setup.sh left eno1
   ```

5. In the `left` container, which serves as the bridge to the outside world,
   assuming `10.0.1.100` is the new IP address and `10.0.1.1` is the gateway.

   ```shell
   docker exec -it left bash
   ip addr add 10.0.1.100/24 dev eth0
   ip route add default via 10.0.1.1 dev eth0
   ```

6. In the `right` container, assuming `10.0.1.101` is the new IP address and
   `10.0.1.1` is the gateway.

   ```shell
   docker exec -it right bash
   ip addr add 10.0.1.101/24 dev eth0
   ip route add default via 10.0.1.1 dev eth0
   ```

7. From the ns-3 directory, start the program that bridges the two TAP devices.

   ```shell
   ./ns3 run scratch/tap-csma-virtual-machine
   ```

8. Start a Chromium browser inside a container. The container has no sandbox
   namespace of its own, hence `--no-sandbox`.

   ```shell
   chromium --no-sandbox --disable-dev-shm-usage
   ```

9. Tear everything down and give the host back its interface.

   ```shell
   sudo bash destroy.sh
   sudo bash internet_reset.sh eno1
   ```

## Capturing traffic

`tap-csma-virtual-machine.cc` writes `tap-csma-virtual-machine-0-0.pcap` and
`tap-csma-virtual-machine-1-0.pcap` in the directory ns-3 runs from, one per
side of the CSMA channel.

```shell
tshark -r tap-csma-virtual-machine-0-0.pcap
tshark -r tap-csma-virtual-machine-0-0.pcap -Y icmp
tshark -r tap-csma-virtual-machine-0-0.pcap -q -z io,phs
```

To capture on the host instead, capture on a TAP device or a bridge port, not
on the bridge itself:

```shell
tshark -i tap-left -w tap-left.pcap
```

Live capture needs `CAP_NET_RAW`. Rather than running tshark as root, add
yourself to the group that owns `dumpcap` once and log back in:

```shell
sudo usermod -aG wireshark "$USER"
```

A Linux bridge switches unicast frames from port to port without passing them
up to the bridge device, so a capture on `br-left` shows broadcast and
multicast only and misses the traffic under test.

## Possible show-stoppers

- On an `XDG_RUNTIME_DIR` error, check that `DISPLAY` is set in the shell that
  runs `setup.sh`, since that value is what gets passed into the containers.
- Under Wayland, the containers reach the display through XWayland, so
  `/tmp/.X11-unix` must exist on the host.
- This setup is sensitive to memory bandwidth: on a slow machine, video playback
  through the emulated network drops frames.
- `br_netfilter` must be loadable for the bridges to pass the emulated traffic
  unfiltered. `single_setup.sh` warns if it is not.
- macOS has dropped its native TUN/TAP support, so the host must be Linux.

## Video demonstration

A screencast of the setup in use is available
[on YouTube](https://www.youtube.com/watch?v=itMjgpfWXYg).
