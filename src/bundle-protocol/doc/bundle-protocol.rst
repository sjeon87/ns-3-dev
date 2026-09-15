.. include:: replace.txt
Bundle Protocol Version 7 (BPv7)
--------------------------------

This model implements the base specification of the Bundle Protocol (version 7). The implementation is based on :rfc:`9171` and :rfc:`5326` for the LTP convergence layer adapter.

The model was written by Ishaan Lagwankar from Michigan State University, and is an updated version of the work done by Ruben Martinez-Vidal and Dizhi Zhou on their initial implementations of the Bundle Protocol and LTP.

Model Description
*****************

The source code for the BP model lives in the directory ``src/bundle-protocol``.

This module provides a native BPv7 implementation for |ns3|, enabling simulation of Delay-Tolerant and Disruption-Tolerant Networks (DTNs) — networks characterized by intermittent link failures, high end-to-end latency, and asymmetric communication channels [Cerf2007]_. These arise prominently in deep-space contexts such as the Interplanetary Network (Solar System Internet), where orbital geometry, solar interference, and propagation physics cause frequent link disruptions [Hylton2022]_.

The implementation provides a full protocol stack conforming to the BPv7 standard [RFC9171]_, including UDP, TCP, and LTP convergence layer adapters, a persistent bundle storage engine, and an extensible Contact Graph Routing (CGR) engine. It has been validated for interoperability with the High-Rate Delay Tolerant Networking (HDTN) implementation [HDTN2024]_, and is available on the ns-3 GitLab as merge request !2826 [MR2826]_.

Design
++++++

The BP class suite consists of five main components:

**Bundle**

A ``Bundle`` comprises an ordered sequence of ``BundleBlock`` instances, beginning with a ``PrimaryBlock`` followed by one or more ``PayloadBlock`` instances, conforming to :rfc:`9171`. The Primary Block header carries routing metadata: destination EID, source EID, report-to EID, creation timestamp, and bundle processing control flags. The entire Bundle is CBOR-encoded as an indefinite-length array [CBOR2013]_.

``BundleBlock`` is implemented as an abstract base class with concrete subclasses for the Primary Block and Payload Block, each with a corresponding header encoded per :rfc:`9171`. Payloads are represented as ``ns3::Packet`` objects, serialized and deserialized at the convergence layer boundary. Extension blocks are left as future work.

**StorageEngine**

The ``StorageEngine`` provides the store-and-forward behavior central to BPv7. Bundles that cannot immediately be forwarded are held in memory as a hash table mapping destination EIDs to bundle queues. When a contact opens, the ``StorageEngine`` retrieves the queue for the corresponding destination and passes bundles to the ``Agent`` for transmission. If the contact closes before the queue is drained, remaining bundles are returned to storage.

Note that once a bundle is passed to the CLA, responsibility for retransmission transfers to the underlying transport protocol; the bundle is not returned to the ``StorageEngine`` regardless of whether delivery is ultimately confirmed. A file-backed ``StorageEngine`` is left as future work.

**Convergence Layer Adapters (CLAs)**

The ``BundleCla`` is an abstract interface that serializes a ``Bundle`` into an ``ns3::Packet`` and manages the corresponding socket lifecycle. CLA instances are registered with the ``Agent`` when a contact opens and deregistered when it closes, driven by the ``ContactPlan`` parsed by the CGR engine. Three concrete implementations are provided:

- ``UdpBundleCla`` — wraps ``UdpSocketFactory`` for best-effort delivery.
- ``TcpBundleCla`` — wraps ``TcpSocketFactory`` for reliable, ordered delivery over terrestrial links.
- ``LtpBundleCla`` — implements the Licklider Transmission Protocol (:rfc:`5326`) over UDP, providing reliable delivery via selective ARQ for high-latency, disruption-prone links. The LTP engine segments data into *red* (reliable, acknowledged) and *green* (best-effort) parts. Outgoing data is segmented into MTU-sized chunks of 1472 bytes, accounting for IPv4 and UDP overhead. Checkpoint retransmission timers are set to twice the one-way light time plus twice the local processing delay plus a one-second margin [Martinez2015]_.

**Contact Graph Routing (CGR)**

The CGR engine maintains a ``ContactGraph`` derived from a contact plan — a schedule of all anticipated contacts in the network. Each contact entry specifies the two nodes involved, the start and end time of the contact window, and the planned data rate in bytes per second. Range entries additionally specify the one-way propagation delay in light-seconds.

Contact plans are parsed from a CSV file formatted per the DTN ION convention [ION2007]_::

    a contact +0 +10000 ipn:1.1 ipn:2.1 2000000
    a range   +0 +10000 ipn:1.1 ipn:2.1 600.0
    a contact +0 +10000 ipn:2.1 ipn:1.1 2000000
    a range   +0 +10000 ipn:2.1 ipn:1.1 0.01

The ``ContactParser`` class ingests this format and schedules ``LinkUp`` and ``LinkDown`` events at the appropriate simulation times. Supporting a new contact plan format requires only a new ``ContactParser`` implementation with no changes to the CGR engine or ``Agent``.

Routing decisions are made via ``GetNextBestHop``, implemented as an abstract interface to allow drop-in replacement of routing algorithms. Three concrete implementations are provided:

- ``PerPacketDijkstraCGR`` — invokes Dijkstra's single-source shortest path algorithm on each transmitted packet.
- ``PerContactDijkstraCGR`` — recomputes shortest paths on each topology change (contact addition or removal) and caches results.
- ``ContactMultigraphRouting`` (CMR) — models the network as a multigraph with time-varying edges and uses a time-aware Dijkstra's algorithm to compute earliest arrival times. This is the algorithm used in HDTN [HDTN2024]_.

Each algorithm tracks link volume as the link data rate multiplied by the contact duration. Every time a packet is sent, the link volume decreases by the size of the packet.

**Agent**

The ``BundleAgent`` is the central controller for all bundle processing. It implements the following operations from :rfc:`9171`: administrative record generation, bundle transmission, dispatching, forwarding, expiration, reception, local delivery, deletion, fragmentation, and ADU reassembly. Cancellation is left as future work.

**Fragmentation and reassembly**

Bundle fragmentation (:rfc:`9171`, Section 5.9) is triggered by the ``FragmentationMtu`` attribute on ``BundleAgent`` (default 0, disabled). If a payload passed to ``TransmitBundle`` exceeds ``FragmentationMtu`` bytes and the ``NO_FRAGMENT`` processing flag is not set, the agent splits it via ``Bundle::Fragment`` into multiple fragment bundles, each an independent, fully routable ``Bundle`` with the ``IS_FRG`` processing flag set and the fragment offset and total application data unit (ADU) length populated on its Primary Block. Each fragment is stored, scheduled for expiry, and forwarded exactly as a whole bundle would be; intermediate nodes require no special handling, since a fragment is just an ordinary bundle.

On reception, a fragment destined for the local node is buffered by the ``BundleAgent`` in a reassembly map keyed by the subject bundle's source EID, creation timestamp, and sequence number -- the same identity used for administrative reporting. Once all bytes of the ADU have been received (no gaps between offsets), the agent reconstructs a single non-fragment ``Bundle`` and delivers it through the normal local-delivery path. Incomplete reassembly buffers are dropped once the fragment's lifetime elapses, mirroring whole-bundle expiry. This fragmentation trigger is a static, payload-size threshold rather than a live check against per-contact channel capacity; see Scope and Limitations.

The send path proceeds as follows:

1. A BP-compliant application passes an ``ns3::Buffer`` and destination URI to the ``Agent``.
2. The ``Agent`` constructs a ``Bundle`` and populates the ``PrimaryBundleBlock`` headers.
3. The ``Agent`` calls ``GetNextBestHop()`` to determine the next forwarding node.
4. The ``Agent`` checks its CLA hash table for a registered CLA for that hop.
5. If no hop or CLA is available, the bundle is stored in the ``StorageEngine`` and an expiry timer is started.
6. If a CLA is available, the ``Agent`` serializes the bundle into an ``ns3::Buffer`` and invokes the CLA.
7. The CLA segments the buffer into MTU-sized packets and transmits them, invoking a success callback to the ``Agent`` on completion.
8. When a new contact is registered, the ``Agent`` scans the ``StorageEngine`` for any bundles queued for the newly reachable destination and initiates their transmission.

Usage
+++++

**Installing agents on nodes**

Use ``BundleAgentHelper`` to install a ``BundleAgent`` on one or more nodes. Each node must be given a unique endpoint ID (EID) before installation::

    BundleAgentHelper agentHelper;
    agentHelper.SetBpEndpointId("dtn://earth/dsn");
    BundleAgentContainer agents = agentHelper.Install(nodes.Get(0));
    Ptr<BundleAgent> agent = agents.Get(0);

**Configuring the routing engine**

Use ``ContactGraphHelper`` to parse an ION-format contact plan and instantiate a CGR engine. The helper accepts the path to the contact plan CSV and the TypeId string of the desired routing engine::

    ContactGraphHelper cgrHelper;
    cgrHelper.SetContactPlan("src/bundle-protocol/examples/contactGraph.csv");
    cgrHelper.SetRoutingEngine("ns3::ContactMultigraphRouting");
    Ptr<BaseRoutingEngine> contactGraph = cgrHelper.Install();
    agent->SetContactGraph(contactGraph);

The contact windows returned by ``contactGraph->GetContactWindows()`` can be iterated to schedule ``LinkUp`` and ``LinkDown`` events for each ``PointToPointNetDevice`` and CLA pair.

**Installing CLAs**

Use ``BundleClaHelper`` to install concrete CLA objects on nodes::

    BundleClaHelper claHelper("ns3::LtpBundleCla");
    BundleClaContainer clas = claHelper.Install(nodes);

CLAs must be set up with local and remote socket addresses, and an ``OnewayLightTime`` attribute for LTP retransmission timing::

    claI->SetAttribute("OnewayLightTime", TimeValue(linkDelay));
    claI->Setup(node, localAddr, remoteAddr);
    claI->SetRxCallback(MakeCallback(&BundleAgent::RecvBundle, agent));

CLAs are registered and unregistered with the ``Agent`` as contacts open and close::

    agent->RegisterCla(destEID, cla);
    agent->UnregisterCla(destEID);

**Sending bundles**

Once an agent has a registered CLA for a destination, bundles are sent by passing a raw data buffer, destination EID, report-to EID, TTL, and processing flags::

    std::vector<uint8_t> payload(100, 0xAA);
    agent->TransmitBundle(destEID, "dtn:none", payload.data(), payload.size(), Hours(24), 0);

If no CLA is registered for the next hop at send time, the bundle is held in the ``StorageEngine`` and forwarded automatically when a matching CLA is later registered.

The ``LtpBundleCla`` is recommended for deep-space scenarios requiring reliable delivery over high-latency links. The ``UdpBundleCla`` and ``TcpBundleCla`` are appropriate for terrestrial or hybrid network segments.

Examples
++++++++

**Mars Relay Network** (``examples/mrn-example.cc``)

This example simulates a data transmission across the Mars Relay Network (MRN), modelling nine nodes: the Deep Space Network (DSN) on Earth and eight Mars assets including orbital relays (MRO, Odyssey, MVN, TGO) and surface vehicles (MSL, M2020, InSight, Ingenuity).

All inter-node links are provisioned as ``PointToPoint`` links initialized at 1 bps. The ``ContactGraphHelper`` parses a pre-computed contact plan CSV and schedules ``LinkUp`` and ``LinkDown`` events that dynamically update each device's data rate and propagation delay for the duration of the contact window. All nodes use the ``LtpBundleCla`` and the ``ContactMultigraphRouting`` engine.

At *t* = 0, the DSN node transmits a burst of 500 bundles (100 bytes each, TTL 24 hours) destined for the Ingenuity helicopter. A storage monitor samples the ``StorageEngine`` size on key relay nodes every 500 simulated seconds to measure custody backlog. The simulation runs for 86400 simulated seconds (24 hours).

To run the example::

    ./ns3 run mars-relay-network-example

Key output metrics printed at the end of the run:

- Total bundles sent and received, and delivery ratio.
- Average one-way time (OWT) in seconds.
- Per-node average bundle backlog (Earth DSN, Mars TGO, Perseverance).

A representative result from the paper: 468 of 500 bundles delivered (93.6%), average OWT of 6500.43 seconds, driven primarily by store-and-forward wait time rather than propagation delay.

Helpers
+++++++

``BundleAgentHelper``
    Installs a ``BundleAgent`` on one or more nodes. Use ``SetBpEndpointId`` to assign the local EID before calling ``Install``. Returns a ``BundleAgentContainer`` whose elements can be iterated or retrieved by index. Multiple agents can be accumulated across containers using ``Add``.

``BundleClaHelper``
    Installs a concrete ``BundleCla`` implementation on one or more nodes. The desired CLA type is specified at construction as a TypeId string (e.g., ``"ns3::LtpBundleCla"``). Attributes of the underlying CLA object can be set via ``SetAttribute`` before installation. Returns a ``BundleClaContainer``.

``ContactGraphHelper``
    Parses an ION-format contact plan CSV via ``SetContactPlan`` and instantiates the specified CGR engine via ``SetRoutingEngine``. Calling ``Install`` returns a ``Ptr<BaseRoutingEngine>`` pre-populated with all contact windows. The contact window list can be retrieved from the engine and iterated to schedule per-link ``LinkUp`` / ``LinkDown`` events on the simulator queue.

Validation
**********

Unit tests
++++++++++

The following test suites are registered and can be run with ``./test.py`` or ``./ns3 test``.

**bundle** (``test/bundle-test.cc``)

Tests the ``Bundle`` object model in isolation. Verifies that a ``PrimaryBlock`` added to a ``Bundle`` correctly exposes all header fields through the bundle's accessor methods, and that a ``PayloadBlock`` added separately correctly stores and retrieves its payload packet and block number.

**bundle-header** (``test/bundle-header-test.cc``)

Tests round-trip serialization and deserialization of all three header types via ``ns3::Packet``:

- ``PrimaryBlockHeader`` — verifies version, processing flags, CRC type, creation time, lifetime, sequence number, destination EID, source EID, and report-to EID for a serialize/deserialize cycle.
- ``PayloadBlockHeader`` — verifies block type, block number, processing flags, CRC type, and block length.
- ``BundleStatusReport`` — verifies status flags, reason code, source EID, fragment offset, bundle receipt time, creation time, and sequence number.

**bundle-storage-engine** (``test/bundle-storage-engine-test.cc``)

Tests the ``BundleStorageEngine`` API:

- Stores a bundle and verifies it is retrievable by handle and that ``GetCurrentSize`` is updated.
- Confirms that ``RetrieveBundle`` returns ``nullptr`` for an invalid handle.
- Verifies that a third bundle is rejected when the engine is at capacity and that the size does not change.
- Confirms ``GetHandlesForDestination`` returns the correct set of handles for a known EID and an empty vector for an unknown EID.
- Verifies that ``DeleteBundle`` decrements the size and removes the entry, and returns a non-zero error code for an invalid handle.

**bundle-fragmentation** (``test/test-bundle-fragmentation.cc``)

Tests bundle fragmentation and reassembly:

- ``Bundle::Fragment`` splits a payload into the expected number of fragments, each carrying the ``IS_FRG`` flag, a correct fragment offset, and the total ADU length; concatenating fragment payloads in order reproduces the original payload.
- ``BundleAgent`` end-to-end: a sender with ``FragmentationMtu`` set below the payload size fragments a bundle into the expected number of packets sent to a mock CLA; a receiver fed those fragments out of order and with gaps does not deliver until the set is complete, then delivers exactly once with the correctly reassembled payload.
- The ``NO_FRAGMENT`` processing flag suppresses fragmentation, even when the payload exceeds ``FragmentationMtu``.

**bundle-agent** (``test/bundle-agent-test.cc``)

Tests the ``BundleAgent`` routing and store-and-forward logic using mock CLA and routing engine stubs:

- Local delivery: a bundle addressed to the agent's own EID is delivered immediately via the receive callback without going through a CLA.
- Direct forwarding: a bundle addressed to a node with a registered ``MockBundleCla`` is forwarded to that CLA; the storage engine remains empty.
- Deferred forwarding: a bundle addressed to a node with no registered CLA is held in the storage engine; when a CLA for that destination is subsequently registered, the backlog is processed automatically and the storage engine is emptied.
- Bundle expiry: a bundle with a short TTL stored in the engine is removed by the expiry mechanism after the simulation clock advances past its lifetime.

**ltp-protocol** (``test/ltp-protocol-test.cc``)

Four test cases covering the LTP convergence layer:

- ``LtpHeaderTestCase`` — verifies serialization size and round-trip correctness for ``LtpHeader``, ``LtpTrailer``, and ``LtpContentHeader`` across all segment types (RD, GD, RS, RD_CP_EORP, RAS, CS, CAS), including session IDs and header/trailer extensions.
- ``LtpQueueSetTestCase`` — verifies that the LTP priority queue dequeues packets in the correct order: report segments first, then red data, then green data.
- ``LtpSessionStateRecordTestCase`` — verifies session ID generation bounds, reception claim insertion (including duplicate rejection and serial number increment), checkpoint and report serial number ranges, and suspend/resume timer behavior for CHECKPOINT, REPORT, and CANCEL timer codes.
- ``BundleAgentLtpClaTestCase`` — verifies that ``RegisterCla`` returns true on first registration and false on a duplicate, that a second EID can be registered independently, and that ``UnregisterCla`` succeeds.

**tcp-bundle-cla** (``test/tcp-bundle-cla-test.cc``)

Instantiates two ``TcpBundleCla`` objects on a single loopback node, sends a serialized ``Bundle`` from one to the other over TCP, and verifies that the receive callback fires exactly once.

**udp-bundle-cla** (``test/udp-bundle-cla-test.cc``)

Instantiates two ``UdpBundleCla`` objects on a single loopback node, sends a serialized ``Bundle`` from one to the other over UDP, and verifies that the receive callback fires exactly once.

Scope and Limitations
+++++++++++++++++++++

The following features of :rfc:`9171` are not yet implemented:

- **Capacity-aware fragmentation** — fragmentation is triggered by a static ``FragmentationMtu`` attribute rather than the actual available channel capacity during a contact window. Users must size ``FragmentationMtu`` relative to the expected contact capacity in their topology; en-route (as opposed to source-node) fragmentation is not performed.
- **Extension blocks** — hop-count limiting and bundle age expiry as defined in the standard are not enforced.
- **CRC validation** — corrupted blocks are not detected at the BP layer; corruption surfaces only at the application.
- **Cancellation and per-bundle-ID deletion** — bundle lifecycle management is limited to expiry-timer-based removal.
- **Administrative record CBOR encoding and consequence handling** — sending status reports is delegated to the application layer.
- **Multi-application support per node** — each node is assumed to host a single BP application with a unique EID.
- **Bundle-in-Bundle Encapsulation (BiBE)** [BiBE2025]_ — under active development.
- **File-backed StorageEngine** — bundles are held in RAM; storage capacity is bounded by the host machine's available memory.

The module assumes perfect clock synchronization across all nodes. Real deployments tolerate clock drift through range message uncertainty margins, which this implementation does not model. A clock skew extension for |ns3| is described in [Lagwankar2025]_.

Future Work
+++++++++++

- Capacity-aware, en-route fragmentation driven by live per-contact channel capacity.
- Extension block support (hop count, bundle age, previous node).
- CRC validation and bundle flag processing on reception.
- Per-bundle-ID deletion and cancellation requests.
- CBOR encoding and full handling of administrative records (status reports).
- Multi-application (CBHE) support per node.
- LTP convergence layer interoperability with a live HDTN node.
- Bundle-in-Bundle Encapsulation (BiBE).
- File-backed ``StorageEngine`` for large-scale simulations.
- Alternate route computation when link volume is saturated.

References
++++++++++

.. [RFC9171] S. Burleigh, K. Fall, and E. Birrane III, "Bundle Protocol Version 7," RFC 9171, January 2022.

.. [RFC5326] M. Ramadas, S. Burleigh, and S. Farrell, "Licklider Transmission Protocol — Specification," RFC 5326, September 2008.

.. [HDTN2024] S. Booth et al., "High-Rate Delay Tolerant Networking (HDTN) User Guide Version 1.3.0," 2024.

.. [Cerf2007] V. Cerf et al., "Delay-Tolerant Networking Architecture," RFC 4838, April 2007.

.. [Hylton2022] A. Hylton et al., "New Horizons for a Practical and Performance-Optimized Solar System Internet," IEEE Aerospace Conference, 2022.

.. [ION2007] S. Burleigh, "Interplanetary Overlay Network: An Implementation of the DTN Bundle Protocol," 2007.

.. [Martinez2015] R. Martínez-Vidal, T. R. Henderson, and J. Borrell, "Implementation and Evaluation of Licklider Transmission Protocol (LTP) in ns-3," Workshop on ns-3, 2015.

.. [Kortas2023] N. Kortas and T. Recker, "Large-Scale Space Network Simulator for Performance-Optimized DTNs," 15th International Conference on Advances in Satellite and Space Communications, 2023.

.. [MR2826] I. Lagwankar, "BPv7 implementation (RFC 9171, RFC 5326)," ns-3-dev Merge Request !2826. https://gitlab.com/nsnam/ns-3-dev/-/merge_requests/2826

.. [Testing2026] I. Lagwankar, "Bundle Protocol ns-3 Testing," 2026. https://github.com/shaanzie/bundle-protocol-ns3-testing

.. [CBOR2013] C. Bormann and P. Hoffman, "Concise Binary Object Representation (CBOR)," RFC 7049, October 2013.

.. [Hughes2016] S. P. Hughes, "General Mission Analysis Tool (GMAT)," Technical Report, NASA, 2016.

.. [BiBE2025] S. C. Burleigh, C. Caini, and A. Grano, "Bundle in Bundle Encapsulation Overview," ASMS/SPSC 2025.

.. [Lagwankar2025] I. K. Lagwankar and S. S. Kulkarni, "Clock Skew Models for ns-3," ICNS3 2025.

.. [Moy2023] M. Moy et al., "Contact Multigraph Routing: Overview and Implementation," IEEE Aerospace Conference, 2023.
