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

- Testing:

  - We need to implement tests for the protocol to ensure that it works as expected. This includes testing the protocol in various scenarios, such as:

    - Sending packets in a working network

    - Sending packets in the previous network with some broken link

    - Managing packets from an external network (without AODVv2)

    - more...

  - And in all scenarios, verifying:

    - The state update of a route (by events or by timeouts)

    - The packets queue management

    - The route selection (by metrics)

    - more...
