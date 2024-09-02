.. include:: replace.txt

Ad Hoc On-Demand Distance Vector Version 2 (AODVv2)
---------------------------------------

This model implements the base specification of the Ad Hoc On-Demand
Distance Vector Version 2 (AODVv2) protocol. The implementation is based on
:draft:`perkins-manet-aodvv2-04`.

The model was written by Francesco Todino and Tommaso Pecorella of University of Florence,
and is based on the ns-3 AODV model developed by Elena Buchatskaia and Pavel Boyko, ITTP RAS.

Model Description
*****************

The source code for the AODVv2 model lives in the directory `src/aodvv2`.

Design
++++++

Class ``ns3::aodvv2::Aodvv2RoutingProtocol`` implements all functionality of
service packet exchange and is templated to allow ``ns3::Ipv4RoutingProtocol`` and
``ns3::Ipv6RoutingProtocol`` to be used.
The base class defines two virtual functions for packet routing and
forwarding.  The first one, ``ns3::aodvv2::RouteOutput``, is used for
locally originated packets, and the second one, ``ns3::aodvv2::RouteInput``,
is used for forwarding and/or delivering received packets.

Protocol operation depends on many adjustable parameters. Parameters for
this functionality are attributes of ``ns3::aodvv2::Aodvv2RoutingProtocol``.
Parameter default values are drawn from the DRAFT and allow the
enabling/disabling protocol features.

AODVv2 discovers routes on demand.  Therefore, the AODVv2 model buffers all
packets while a route request packet (RREQ) is disseminated.
A packet queue is implemented in aodvv2-rqueue.cc. A smart pointer to
the packet, ``ns3::IpRoutingProtocol::ErrorCallback``,
``ns3::IpRoutingProtocol::UnicastForwardCallback``, and the IP header
are stored in this queue. The packet queue implements garbage collection
of old packets and a queue size limit.

The routing table implementation supports garbage collection of
old entries and state machine, defined in the standard.
It is implemented as a STL map container. The key is a destination IP address.

The model uses the following heuristics:

* This AODVv2 implementation can detect the presence of unidirectional
  links and avoid them if necessary. If the node the model receives an
  RREQ for is a neighbor, the cause may be a unidirectional link.
  This heuristic is taken from AODV implementation and can be disabled.

The layer 2 feedback implementation relies on the ``TxErrHeader`` trace source,
currently supported in AdhocWifiMac only.

Scope and Limitations
+++++++++++++++++++++

The model is for IPv4 and ready to be implemented for IPv6.
The following optional protocol optimizations are not implemented:

#. Local link repair.
#. RREP, RREQ and HELLO message extensions.

These techniques require direct access to IP header, which contradicts
the assertion from the AODVv2 DRAFT that AODVv2 works over UDP.  This model uses
UDP for simplicity, hindering the ability to implement certain protocol
optimizations. The model doesn't use low layer raw sockets because they
are not portable.

Future Work
+++++++++++

No announced plans.

..
  References
  ++++++++++

..
  Usage
  *****

..
  Examples
  ++++++++

..
  Helpers
  +++++++

..
  Attributes
  ++++++++++

..
  Tracing
  +++++++

..
  Logging
  +++++++

..
  Caveats
  +++++++

..
  Validation
  **********
  Unit tests
  ++++++++++
  Larger-scale performance tests
  ++++++++++++++++++++++++++++++

