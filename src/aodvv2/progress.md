# AODVv2 implementation

## Comparison to RFC 3561

AODVv2 operates in a fashion very similar to AODV. The mechanism for route discovery is basically the same, so that similar performance results can be expected.

Compared to AODV, AODVv2 has moved some features out of the scope of the rfc, notably intermediate route replies, and expanding ring search; furthermore, the following mechanisms have changed:

- Verification of link bidirectionality has been improved

- Alternate metrics may be used

- Support for multiple interfaces has been improved

- Support for multi-interface IP addresses has been added

- A new security model allowing end to end integrity checks has been added

- Message formats are now compliant with RFC 5444

- Hello messages and local repair have been removed

- Multihoming is supported

AODVv2 has not been designed to be interoperable with AODV. However, it would be straightforward to allow both protocols to be used in the same ad hoc network as long as compatible metrics were used.

## Things done

- Implementation of the AODVv2 code starting from AODVs code

- Development of a templated version of the AODVv2 code to allow different ip versions

- Implementation of the packetbb format for the AODVv2 packets

- Removed unused code dependencies from AODV

- Implementation of a temporary solution for NS_OBJECT_TEMPLATE_CLASS_NAMESPACE_DEFINE ([issue #1132](https://gitlab.com/nsnam/ns-3-dev/-/issues/1132))

- Inizialitation of the AODVv2 test suite

## Things in progress

- Implementation of data structures (ch. 4)

## Things to do

- Removal of unused code from AODV

- Understand how the code should work where there is a `TODO me:` comment

- Implementation of the AODVv2 Ipv6 code (you can find the todo parts looking for `TODO Ipv6` comments)

- Verification of AODVv2 Protocol Operations (ch. 6)

- Management of external packets (ch. 9)

### Tests:

> Note: SimpleNetDevice / SimpleChannel is either a "P2P" or a "CSMA", depending on how you configure it. You can block the communications between two nodes using SimpleChannel::BlackList. Hence, it's possible to make a network of that looks like an ad-hoc network.

--- 2 nodes (A, B), connected through a SimpleNetDevice / SimpleChannel.

(1a). Sends an UDP packet to B.

- RREQ sent
- RREP sent
- Routing table filled
- Packet received.

(1b). Like 1. but you loose the RREQ

- RREQ is sent again

(1c). Like 1. but you loose a RREP

- Dunno what should happen, but it happens.

(2). A sends two UDP packets to B, 2nd packet sent before receiving a RREP

- RREQ sent
- RREP sent
- Routing table filled
- Both packets received.

(3). Like 2, but send enough packets to fill the sending queue in AODVv2

- RREQ sent
- RREP sent
- Routing table filled
- Some packets received (the number must be predictable).

(4). Like 2, but 2nd packet is sent _after_ receiving the RREP

- RREQ sent (just one)
- RREP sent (just one)
- Routing table filled
- Both packets received.

(5-6). Like 4, but 2nd packet is sent at a time useful to hit the routing table timeouts.

- same checks as above, modified according to the timeouts.

--- 3 nodes (A, B, C), connected through a SimpleNetDevice / SimpleChannel.

(7). Just like 1.

- Check that the middle node caches the route

(8). Like 7, but node B send a packet to C, and after the route is cached, A sends a packet to C

- Check that B replies with the cached route
