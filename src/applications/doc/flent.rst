.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Flent
-----

Flent is a wrapper around netperf (https://hewlettpackard.github.io/netperf/) and similar network benchmarking tools to run predefined tests, 
originally authored by Toke Høiland-Jørgensen `[1]`_. On running any predefined test, flent aggregates 
results into a gzipped file of JSON format. One may open this file with the help of the flent GUI to 
visualize the aggregated results as plots in a very interactive window. 

Inspired by flent, a flent application has been implemented in ns-3 as well.

Model Description
*****************

Just as the flent application on real systems is a wrapper around other traffic benchmarking tools including netperf and ping, the ``FlentApplication`` in ns-3 also wraps
``BulkSendApplication``, and aims to support a subset of flent's tests by also generating a 
JSON trace file that can be read by the flent GUI. Out of the several tests including RRUL tests, 
TCP flow tests, UDP flow tests, VoIP tests, HTTP tests, etc. that Flent supports, the ``FlentApplication`` 
in ns-3 supports the following tests: 

  - Ping Test: This test makes RTT measurements between the host and the remote machine with the help of an ICMP Ping.

  - RRUL (Realtime Response Under Load) Test: This test saturates the bottleneck link between the host and the remote machine with 4 TCP upload, 4 TCP download flows while measuring RTT with the help of an ICMP Ping and 3 UDP Ping flows with different DiffServ classifications.

  - TCP_Upload: This test saturates the bottleneck link between the host and the remote machine with 1 TCP upload flow while measuring RTT with the help of an ICMP Ping.

  - TCP_Download: This test saturates the bottleneck link between the host and the remote machine with 1 TCP Download flow while measuring RTT with the help of an ICMP Ping.

Design
======

The source code for the ``FlentApplication`` is located in the directory ``src/applications/model`` and 
consists of 2 files `flent-application.h` and `flent-application.cc` defining a FlentApplication class.

Although flent will typically gzip the JSON file, the ns-3 JSON file will be uncompressed and parsable by the flent GUI. The flent 
GUI is a part of the installation of flent.

The FlentApplication measures the following metrics: 
  - RTT: Each RTT sample is calculated as the difference of time between the ICMP Echo Request and (if received) ICMP Echo Reply. 
  - Throughput: Each throughput sample is calculated by counting the bytes sent by the sender or receiver in each sampling interval (depending upon if the test is upload or download) and excluding any TCP or UDP (or lower layer) header bytes, and dividing by the sampling interval.

In the actual flent tool, at the start of a test, netperf establishes a TCP control connection 
using BSD sockets. This connection initially helps exchange the details of the test before it is started. 
A separate connection is made while running the  test. At the end of the test, statistics (like bytes 
received, cpu utilization, service demand, time elapsed, cpu rate) about the remote machine are requested 
via the control connection by the sender.

The following points provide useful insights about how flent records metrics:  
  - Flent reuses the definition and implementation of throughput from netperf. It collects the throughput readings that netperf produces. Netperf is a network benchmark tool used to measure the network performance. Flent requires the user to run netserver on the remote machine and netperf on the local machine.
  - The throughput data points are generated on the sender side when the test is running. The sender uses the linux send function `[2]`_ to transmit “send_size” bytes at a time from the tcp socket. When the function returns a value less than send_size, it marks the end of the test. Hence when “send_size” is returned, the sender increments the cumulative bytes sent by send_size everytime it sends a message and records data points for plotting the throughput `[3]`_.  
  - By default, netperf sets the socket send_size as equal to the initial socket buffer size. The socket buffer sizes are dynamic during the execution of the test. ns-3 on the other hand does not have dynamic socket buffer sizes `[4]`_.   
  - The TCP MSS used by netperf is read from the kernel. 
  - It uses socket stats (‘ss’) to measure congestion window, smooth RTT, pacing rate and delivery rate. 
  - It uses ‘tc’ to measure queue delay, queue backlog, drops,and marks. 
  - Since ‘ss’ and ‘tc’ sample data at regular intervals that cannot be less than the execution time of ‘ss’ and ‘tc’ itself (which may vary depending upon the hardware). However ns3 collects data with help of writing into trace files via periodic callbacks. These callbacks have practically negligible execution time. Hence one may expect to see different results for smaller sampling rates while comparing ns3 and flent. 

References
==========
\

.. _`[1]`:

[1] Flent Documentation: https://flent.org/intro.html

\ 

.. _`[2]`:

[2] Man Page, Linux Send Function: https://man7.org/linux/man-pages/man2/send.2.html

\

.. _`[3]`:

[3] Netperf Github Repository: https://github.com/HewlettPackard/netperf/blob/HEAD/src/nettest_bsd.c#L2212-L2228

\

.. _`[4]`:

[4] Flent Github Repository: https://github.com/tohojo/flent/blob/HEAD/flent/settings.py#L371-L379


Usage
*****

FlentApplication is similar to other ns-3 applications in its usage and configuration, with a helper 
(FlentHelper) and allowing the user to use attributes to configure values away from defaults.The 
flent-example.cc program may be consulted to see the basic usage of this flent application. 

The configurable parameters of flent application are as follows: 
  - TestName: The name of the test to be performed. This shall be one of the predefined tests supported in the flent application. 
  - HostAddress: The address of the remote host.
  - LocalBindAddress: The address of the local host.
  - ImageText: The text to be included in the plot.
  - ImageName: Name of the image to save the output plot.
  - StepSize: The sampling rate that flent application should use. 

Examples
========
An example demonstrating the usage of flent application is ``flent-example.cc`` which is present in 
``src/applications/examples/``. The topology of this example looks like this: 
	                     	           
Client <- - - 5000Mbps,1us - - -> router1 <- - - 50Mbps,5ms - - -> router2 <- - - 5000Mbps,1us - - -> Server

where the bottleneck is between router1 and router2. The default bandwidths are represented in the diagram. 
It can be run as follows: 
run::

  $ ./ns3 --run 'flent-example'

The parameters for the example are as follows: 
  - test: Type of ns-3 flent test
  - rtt: Delay value
  - bw: Data Rate
  - delay: Time to delay test (--delay in flent)
  - verbose: To enable logging during the test. The logs will contain events including the sending and receiving of ICMP pings, UDP Pings and 	TCP Packets. 
 
The output produced by this example is a ‘.flent’ file that is of JSON format. However, not all keys in the 
JSON file have been populated (like various fields in the metadata) and have been left null because they are not 
needed for generating plots. Any user may parse this JSON file either as a text output file or view time 
series plots with the help of flent GUI (Since this file is compatible with flent GUI). To use the flent GUI, 
one may install the flent application, use its GUI and view time series plots of throughput and RTT. 

Tests
*****

Validation
==========

Regression Tests
================

