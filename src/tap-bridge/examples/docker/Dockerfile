# Container image for the ns-3 tap-bridge emulation example.
#
# Build it as the tag the setup scripts expect:
#   docker build -t ns3exp:latest .

FROM debian:trixie-slim

# iproute2      configure eth0 inside the container
# iperf3        throughput measurements across the emulated network
# iputils-ping  reachability checks
# wget          fetch content over the emulated network
# xauth         X11 client support, so GUI applications can use the host display
# chromium      a real application to drive traffic
RUN apt-get update \
    && apt-get install --no-install-recommends -y \
        chromium \
        iperf3 \
        iproute2 \
        iputils-ping \
        wget \
        xauth \
    && rm -rf /var/lib/apt/lists/*

ENTRYPOINT [ "/bin/bash" ]
