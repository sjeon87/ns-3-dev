IEEE 802.3: Ethernet
====================

.. include:: replace.txt
.. highlight:: cpp

This chapter describes the implementation of |ns3| models for the
Switched Ethernet based on IEEE 802.3 standards. The model provides
support for full-duplex switched Ethernet communication and follows
the layered architecture of Ethernet by separating the NetDevice,
MAC, PHY, and Channel components.

The model is implemented into the ``src/ethernet`` folder.

The functionality of the Ethernet model is divided among the layers of the
OSI model to achieve a clear separation of responsibilities and modular design.

Ethernet Channel
----------------

The EthernetChannel provides the communication medium between ``EthernetNetDevice``
instances. The channel is responsible for connecting two devices, modeling packet
propagation delay, and delivering packets between attached devices.

The channel maintains a list of connected devices in ``m_deviceList``.
There is a ``PropagationStart`` which starts the propagation of data on the wire and
a ``TxEnd`` method which is called when PHY layer has finished transmitting
packet on the channel.

The channel models the cable, and the only cable property it enforces is length:
the ``Length`` attribute cannot exceed the 100 m maximum defined for Ethernet over
twisted-pair copper. The propagation delay is computed from ``Length`` and
``Speed``.

Utility methods such as GetNDevices(), GetDevice() and GetDelay() provide access to device information and
channel configuration parameters.

When the first device is attached, it operates at its own maximum supported link
type. When the second device is attached, or when the maximum supported link type
of either device changes, the channel renegotiates the actual link type for both
devices as the slower of the two supported link types. The negotiated result is
stored on the devices and is later used by the PHY layer to compute transmission
time and inter-frame gap duration.

Detaching a device removes it from the channel, marks the remaining device link
down, and causes link-type negotiation to be recomputed for the device that
remains attached.

Link speed selection
~~~~~~~~~~~~~~~~~~~~

The nominal data rate of a link is given by an ``EthernetLinkType``, which
corresponds to a data rate defined by the IEEE 802.3 standards. Every
``EthernetNetDevice`` declares the fastest link type it is able to run at through
its ``MaxSupportedEthernetLinkType`` attribute.

IEEE 802.3 autonegotiation is not modelled. Instead, ``EthernetChannel`` settles the
link type of both attached devices on the slowest of the two maximum supported link
types, which is the outcome autonegotiation would arrive at for a full-duplex link.
This happens in ``EthernetChannel::NegotiateLinkType``, which is invoked whenever a
device is attached or detached, or when the maximum supported link type of an
attached device changes. The result is readable from each device through
``GetActualLinkType()``, and the corresponding data rate through ``GetDataRate()``.
While only one device is attached to a channel, it runs at its own maximum supported
link type.

The PHY does not hold a rate of its own; it carries whatever rate the device has
settled on, and uses it to compute frame transmission times and the inter-frame gap.

Ethernet PHY
------------

The EthernetPhy layer provides the physical transmission behavior and exposes transmit and receive callbacks
to interact with the channel. It handles transmission timing, link state management, and error modeling,
but does not interpret packet contents. The PHY is agnostic to the link speed: it carries whichever data rate
the device it belongs to has settled on with its peer.

The PHY maintains internal state information for both transmission and reception paths using an ``EthernetPHYState`` enum,
which includes idle, transmitting, and receiving states. These states ensure proper coordination of operations and prevent
conflicts such as simultaneous transmission and reception.

The PHY stores pointers to its device, MAC, and channel. ``TxStart()`` verifies that
those pointers are available and that the link is up before starting transmission.
It then traces transmission begin, schedules transmission completion based on the
negotiated data rate, and asks the channel to start propagation. ``TxEnd()`` marks
the PHY idle again, emits the transmission-end trace, informs the MAC that the
frame has finished, and tells the channel that transmission has completed.

On the receive path, ``Receive()`` checks link state, applies the optional receive
error model, and forwards a copy of the packet to the MAC only when reception is
successful.

Ethernet NetDevice
------------------

The EthernetNetDevice provides the interface to higher protocol layers. It is a
thin wrapper around the Ethernet MAC, PHY, and Channel objects, exposing the
standard ``NetDevice`` API expected by the rest of |ns3|.

The ``Send()`` method is used by higher layers to transmit packets using the
device's own source address, while ``SendFrom()`` allows explicit specification of
the source MAC address.

On the receive path, packets that successfully pass the PHY and MAC processing
path are delivered to the registered ``ReceiveCallback`` or, when applicable, the
``PromiscReceiveCallback``. The device itself does not parse Ethernet frames; it
simply forwards packets between the MAC layer and upper-layer protocol handlers.

The device stores the negotiated link state reported by the channel and exposes it
through ``GetActualLinkType()`` and ``GetDataRate()``. If the channel is not
attached yet, the device runs at its configured maximum supported link type.

Support for full-duplex mode is provided. The device simply hands packets down to
the MAC and is called back when a frame is to be delivered up.

Ethernet MAC
------------

The EthernetMAC class represents the Medium Access Control (MAC) sublayer of an Ethernet network interface.
It is responsible for providing frame-level communication services between the higher network layers (via the NetDevice)
and the physical transmission medium (via the PHY layer).

The EthernetMac layer is responsible for Ethernet frame processing. It adds the Ethernet header to packets received from
the EthernetNetDevice and initiates transmission by invoking the PHY transmit callbacks. Similarly, it processes incoming
frames from the PHY layer and forwards valid packets to the upper layers.

The MAC sublayer owns the tx and rx queues, and the state machines that
drive them. A packet handed down by the device is encapsulated into a complete frame
and, if the transmitter is idle and not paused, given straight to the PHY; otherwise
it waits in the tx queue, which is drained as each transmission and the following
inter-frame gap complete. On the receive side, a frame arriving from the PHY is
processed immediately if the receiver is idle, and queued otherwise; the receiver
is released after a configurable processing delay, at which point the next queued
frame is taken. A frame that arrives when the receive queue is full is dropped, and
makes the MAC ask the peer to pause.

The MAC layer also maintains internal state information ``EthernetMACState``. Additionally, EthernetMAC supports promiscuous
and non-promiscuous reception modes, enabling both standard packet delivery to upper layers and packet sniffing functionality
for monitoring purposes.

Flow Control
~~~~~~~~~~~~

The MAC layer implements IEEE 802.3x Ethernet Flow Control for full-duplex links.
Flow control messages are carried using the ``FlowControlHeader``. For PAUSE
operation, the FlowControlHeader carries a PAUSE message containing the requested
pause time encoded as a 16-bit pause quanta value, where one pause quantum corresponds
to 512 bit times. When receive-buffer congestion is detected, the MAC transmits a
PAUSE frame to the remote peer. Upon receiving a PAUSE frame, the MAC pauses normal
frame transmission and automatically resumes transmission after the pause interval
expires.

The implementation currently uses queue occupancy thresholds to trigger flow
control. When the receive queue reaches 80 percent of its configured capacity, the
MAC sends a PAUSE frame once, and it resumes the peer only after the queue drains
to 50 percent or below. The pause duration is derived from the negotiated data
rate so that the encoded pause quanta maps to simulation time using the link
actually in use. This keeps the flow-control behavior local to the MAC and makes
the thresholds and resume policy easy to replace later without changing the rest
of the model.

Ethernet Switch
---------------

The ``EthernetSwitch`` class models a learning Ethernet switch. It is aggregated
to a node and manages a set of ports, one for each link.The switch controls when
a received frame is removed from the port's Rx queue. If the switch cannot currently
accept the frame, the frame remains in the ingress receiRxve queue and is retried
later.

For every frame it receives, the switch associates the source address with the
port the frame came in on, and looks the destination address up in the same
table. A frame whose destination address is known is forwarded to the port that
address was learned on, unless that is the port the frame came in on, in which
case it is discarded. A frame whose destination address is unknown is flooded to
every port other than the one it came in on. Learned addresses are forgotten
after ``ExpirationTime``.

Buffering and scheduling
~~~~~~~~~~~~~~~~~~~~~~~~

Frames are not handed to an output port as soon as they are switched. They are
placed in a buffer owned by the switch, out of which an
``EthernetSwitchScheduler`` picks them and hands them to the ports, one
transmission per port at a time. ``EthernetSwitchFcfsScheduler`` serves them in
the order they were received.

Because the switch does not own the queues of its ports, it cannot hand a frame
to a port whose Tx queue is full. ``EthernetSwitch::CanForwardTo()`` is the
single point at which the room left on an output port is established, and the
scheduler consults it before handing a frame over. What is done with a frame
that its output port cannot take is left to the scheduler: serving frames in the
order they arrived leaves the FCFS scheduler nothing else to do than drop them,
which it reports through the ``ForwardDrop`` trace source, whereas a scheduler
free to serve them in another order can hold them back in the buffer until the
port drains.

The switch also controls when frames are removed from the ingress Rx
queues. Before dequeuing a frame, ``EthernetSwitch::CanAccept()`` checks whether
the switch can accept it, including the optional switch memory policy. If the
memory check rejects the frame, EthernetSwitch::ReceiveFromPort() leaves
the frame in the ingress RX queue, providing backpressure until the switch can
accept it.

Queue memory model
~~~~~~~~~~~~~~~~~~

The transmit and receive queues of a MAC each have their own configured queue
limits. The available space in a queue is determined by its configured limit.

Real switches often organise their queue memory differently. The transmit and
the receive queue of a port may draw on a pool shared between them, so that the
split between the two directions is free to vary as long as their total stays
within one limit; or all the ports may draw on a pool shared switch-wide. Under
either of those, the limit that decides whether a frame is accepted is no longer
the limit of the queue it would be placed in, and asking the queue whether it is
full stops being meaningful.

Neither shared-memory model is enforced by default. The memory-check callbacks
provide an extensible mechanism for modelling such policies without reworking
the switch or changing the port queues. The forwarding memory check can be used
to account for additional memory constraints beyond the port queue limits,
while the switch memory check can be used to impose additional constraints on
frames entering the switch. No additional memory limit is enforced unless the
corresponding callback is configured.

Scope and Limitations
---------------------
- The model currently supports Ethernet over twisted-pair copper links operating in full-duplex mode; support for fiber-optic and other link types is planned for future work.
- Link speed currently uses an almost auto-negotiation approach: each EthernetNetDevice specifies a maximum link speed, and once two devices are connected to the same channel,
their operating speed is set to the minimum of the two maximum speeds; this implicitly assumes a copper medium.
- Half-duplex Ethernet and CSMA/CD collision detection are not implemented and are planned for future work.
- Ethernet Flow Control using IEEE 802.3 PAUSE frames is implemented for full-duplex Ethernet links.
- The current implementation uses a simple queue-threshold policy for PAUSE frames. Priority-based PAUSE/advanced queue management is not currently supported and is considered future enhancement.
- The switch models queue memory as a fixed allocation per queue. Ports whose transmit and receive queues share a pool with each other, or with the queues of the other ports, are not supported.
- The buffer the switch holds frames in while they wait for their turn on an output port is unbounded, so the memory of the switch fabric itself is not modelled.
- The switch forwards every frame it receives. VLANs, the spanning tree protocol and the reserved multicast addresses that carry it are not implemented.

Extensibility and Future Enhancements
-------------------------------------

- The current architecture is intentionally layered so that extensions can be added
locally. New link speeds can be introduced by extending ``EthernetLinkType`` and
updating the mapping to ``DataRate`` in the NetDevice implementation. If the
supported rate-selection rules change, the channel negotiation logic can be
updated without affecting the MAC or higher protocol layers, because the PHY
always derives timing from the device's negotiated data rate.

- Future work may allow each device to specify a list of allowed link modes,
such as copper or fiber, enabling the channel to negotiate a compatible medium
and link speed between the connected devices.

- The MAC layer is extensible. The transmit and receive queues are owned
through attributes, so the queue implementation and queue limits can be changed
without rewriting the frame-processing logic.

- The flow-control code is isolated in the MAC, which makes it possible to replace
the current threshold-based PAUSE policy with priority-based flow control,
different pause-release criteria, or additional MAC Control opcodes in the future.

- The switch keeps its forwarding policy, scheduling policy, and memory policies apart.
A new scheduling discipline only has to subclass EthernetSwitchScheduler and can decide
for itself what to do with a frame that its output port cannot accept.
Queue memory organisations beyond the fixed per-queue limits can be modelled through
EthernetSwitch::SetMemoryCheck() for forwarding and EthernetSwitch::SetSwitchMemoryCheck()
for switch-level ingress acceptance, without changing the forwarding or scheduling logic.

- Future work can also extend the PHY layer independently by adding new error
models, alternative link-state behavior, or richer receive timing models. Because
the PHY already interacts with the channel and MAC through explicit callbacks,
those changes can remain local to the PHY implementation.

Usage
-----
This section describes how to use Ethernet module. It covers the Helper class, configurable attributes of each component in module,
available trace sources used for debugging and performance testing.

Helpers
~~~~~~~

The helper follows the standard ns-3 helper pattern. It creates an
``EthernetChannel`` and two ``EthernetNetDevice`` instances, connects both devices
to the same channel, and installs them on exactly two nodes. The helper exposes
factory-style configuration methods for the device and channel before
installation:

* ``SetDeviceAttribute()`` configures the ``EthernetNetDevice`` factory.
* ``SetChannelAttribute()`` configures the ``EthernetChannel`` factory.

The helper also integrates with the standard ns-3 tracing support. PCAP tracing
records Ethernet packets using the Ethernet link type, and ASCII tracing hooks the
MAC receive path together with the transmit and receive queues and PHY receive
tracing (ASCII and PCAP) is enabled in a manner consistent with other network

Use of the helper is demonstrated in the example provided in ``src/ethernet/examples``.
device helpers through the inherited tracing interfaces. The helper is
responsible for creating and configuring ``EthernetNetDevice`` instances,
attaching them to an ``EthernetChannel``, and installing them on the specified nodes.
Use of the helper is demonstrated in the example provided in ``src/ethernet/examples``.

::

    NodeContainer nodes;
    nodes.Create(2);
    EthernetHelper ethernet;
    NetDeviceContainer devices = ethernet.Install(nodes);

As shown above, the helper can be used to create a simple point-to-point Ethernet
link between two nodes. After installation, the returned NetDeviceContainer can
be used together with the Internet stack helpers to assign IP addresses,
configure routing, and run applications.

Attributes
~~~~~~~~~~

The EthernetChannel provides following attributes:

* ``Length``: The length of the channel in meters, up to 100 meters maximum length of an Ethernet cable.
* ``Speed``: The speed of propagation in the channel in meters per second.

The EthernetPhy provides following attributes:

* ``ReceiveErrorModel``: The error model for simulating packet loss on reception.

The EthernetNetDevice provides following attributes:

* ``Mac``: Ethernet MAC associated with the device
* ``Phy``: Ethernet PHY associated with the device.
* ``Channel``: The channel to which the device is attached.
* ``MaxSupportedEthernetLinkType``: The fastest Ethernet link type the device may run at, one of ``10Base-T``, ``100Base-TX``, ``1000Base-T`` or ``10GBase-T``.
* ``SendEnable``: A boolean attribute indicating whether the device is enabled to send packets.
* ``ReceiveEnable``: A boolean attribute indicating whether the device is enabled to receive packets.

The EthernetMac provides the following attributes:

* ``Address``: The Mac48Address of the device.
* ``Mtu``: The maximum transmission unit for the device in bytes.
* ``TxQueue``: The queue holding the frames waiting for transmission.
* ``RxQueue``: The queue holding the received frames waiting to be processed.

The EthernetSwitch provides the following attributes:

* ``ExpirationTime``: The time after which a learned address is forgotten.

Traces
~~~~~~

The following trace sources have been implemented to monitor the behavior of the **PHY layer**:

* ``PhyTxBegin``: Indicates that a packet has begun transmitting over the channel medium.
* ``PhyTxEnd``: Indicates that a packet has been completely transmitted over the channel.
* ``PhyTxDrop``: Indicates that a packet has been dropped by the device during transmission.
* ``PhyRxBegin``: Indicates that a packet has begun being received from the channel medium by the receiver.
* ``PhyRxEnd``: Indicates that a packet has been completely received from the channel medium.
* ``PhyRxDrop``: Indicates that a packet has been dropped by the device during reception.

The following trace sources have been implemented to monitor the behavior of the **MAC layer**:

* ``MacTx``: Indicates that a packet is being transmitted by the MAC layer.
* ``MacTxDrop``: Indicates that a packet has been dropped by the MAC layer before transmission.
* ``MacPromiscRx``: Indicates that a packet has been received at the MAC layer in promiscuous mode.
* ``MacRx``: Indicates that a packet has been received at the MAC layer.
* ``MacRxDrop``: Indicates that a packet has been dropped by the MAC layer during reception.
* ``Sniffer``: Indicates that a packet has been received at the MAC layer in sniffer mode.
* ``PromiscSniffer``: Indicates that a packet has been received at the MAC layer in promiscuous sniffer mode.

The following trace source has been implemented to monitor the behavior of the **switch**:

* ``ForwardDrop``: Indicates that the switch gave up on forwarding a frame to the port it is destined to.

Examples and Tests
------------------

The following examples have been written in ``src/ethernet/examples``.

* ``ethernet-ping.cc``: A simple two-node topology demonstrating basic communication over Ethernet links.
* ``ethernet-switch-ping.cc``: Two hosts pinging each other through a switch, exercising address learning and flooding.

The following unit-tests have been written in ``src/ethernet/test``.

* ``ethernet-full-duplex-test.cc``: A test for verifying that the EthernetChannel can transmit simultaneously in opposite directions and successfully receive each other's packets.
* ``ethernet-ifg-test.cc``: A test for the inter-frame gap functionality in Ethernet.
* ``ethernet-link-speed-test.cc``: A test for verifying the Ethernet PHY transmission and packet reception timing for supported link speeds. .
* ``ethernet-flow-control-test.cc``: Unit test for Ethernet flow control functionality.
* ``ethernet-switch-test.cc``: Unit test for switch address learning, flooding, and the queue limits of the output ports.
* ``ethernet-switch-fcfs-test.cc``: Unit test for the FCFS switch scheduler, verifying that it serves frames in the order they were received and respects the output port queue limits.

References
----------

[`1 <https://ieeexplore.ieee.org/document/30707/>`_] Carrier Sense Multiple Access With Collision Detection (CSMA/CD) Access Method and Physical Layer Specifications, in ANSI/IEEE Std 802.3-1985 , vol., no., pp.0_1-, 1985, doi: 10.1109/IEEESTD.1985.82837.

[`2 <https://tcipg.org/sites/default/files/papers/2010_Jin_Nicol_Caesar.pdf/>`_] Efficient Gigabit Ethernet Switch Models for Large-scale Simulation, Dong Jin, David M. Nicol, and Matthew Caesar.

[`3 <https://ieeexplore.ieee.org/document/9844436>`_] IEEE Standard for Ethernet, in IEEE Std 802.3-2022 (Revision of IEEE Std 802.3-2018) , vol., no., pp.1-7025, 29 July 2022, doi: 10.1109/IEEESTD.2022.9844436.

[`4 <https://ieeexplore.ieee.org/document/1309630>`_] IEEE Standard for Local and metropolitan area networks: Media Access Control (MAC) Bridges," in IEEE Std 802.1D-2004 (Revision of IEEE Std 802.1D-1998) , vol., no., pp.1-281, 9 June 2004, doi: 10.1109/IEEESTD.2004.94569.

