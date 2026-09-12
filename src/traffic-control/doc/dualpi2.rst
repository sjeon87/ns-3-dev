.. include:: replace.txt
.. highlight:: cpp

DualPI2 queue disc
==================

This chapter describes the DualPI2 ([RFC9332]_) queue disc implementation
in |ns3|.

The model provides the necessary node-level infrastructure for the Low Latency,
Low Loss, and Scalable throughput (L4S) architecture. It performs traffic
isolation by steering ECT(1) packets into a dedicated L4S queue, while
maintaining a coupled marking probability between the L4S and Classic queues.
The implementation aims for functional parity with the Linux kernel's
``sch_dualpi2.c``, providing mechanisms such as a credit-based WRR scheduler and
a step AQM marking function to maintain low queuing delay for compatible
L4S flows.

Model Description
*****************

The source code for the DualPI2 model is located in the directory
``src/traffic-control/model`` and consists of 2 files: `dualpi2-queue-disc.h`
and `dualpi2-queue-disc.cc` defining a DualPi2QueueDisc class. The code was
ported to |ns3| by Maria Eduarda Veras, Eduardo Freitas and Djamel Sadok based
on the Linux kernel code [SchDualPI2]_.

This model implements a Dual-Queue framework, providing two internal queues to
isolate L4S traffic from Classic traffic. The L4S queue is intended for ECT(1)
and CE packets, and the Classic queue for Not-ECT and ECT(0) packets. Both
queues are coupled through the PI2 marking/dropping probability, with the L4S
queue using a step AQM on top of the PI2 probability for congestion
signaling. Both queues are drained by a credit-based scheduler, with default
prioritization for the L4S queue.

* class :cpp:class:`DualPi2QueueDisc`: This class implements the main DualPI2
  algorithm:

  * ``DualPi2QueueDisc::DoEnqueue()``: This routine is the entry point for
    incoming packets. It first checks the total byte count against the
    `m_queueLimit`; if the buffer is full, the packet is dropped (overflow). If
    space is available, the routine classifies the packet based on its IP ECN
    field: packets marked as ECT(1) are directed to the internal L4S queue,
    while ECT(0) or Not-ECT packets are directed to the Classic queue. An
    internal ns-3 Tag is then attached to the L4S packet solely to store a
    boolean indicator of whether the Step AQM should be applied during
    dequeue. This mechanism ensures that even if internal queue state changes,
    the packet is processed according to its classification at entry. Finally,
    if the `m_dropEarly` is enabled, the routine may invoke MustDrop() to decide
    whether to discard the packet based on the current PI2 probability.

  * ``DualPi2QueueDisc::DualPi2Update()``: This routine is called at regular
    intervals defined by the `m_Tupdate` attribute. It computes the queuing
    delay as the maximum sojourn time between the head-of-queue packets of both
    internal queues, then updates `m_baseProb` using a PI controller with gains
    A and B, clamping the result to [0, 1]. The Classic drop probability is
    derived as `m_pC` = `m_baseProb` * `m_baseProb` and the L4S mark probability
    as `m_pL` = `m_baseProb` * `k`, coupling both queues through a shared base
    probability.

  * ``DualPi2QueueDisc::MustDrop()``: This routine implements the core signaling
    and overload logic. No action is taken if the total queue size is below two
    MTUs. Overload is detected when `m_baseProb` * `m_k` > 1.0; in this state,
    Classic packets are dropped instead of marked, and L4S packets are dropped
    with probability `m_pC` if `m_dropOverload` is enabled. Under normal
    conditions, L4S packets are probabilistically marked with probability
    `m_pL`, and Classic packets are marked or dropped with probability `m_pC`.

  * ``DualPi2QueueDisc::StepAqm()``: This method applies CE marking to L4S
    packets only. It marks a packet if the enqueue-time tag that indicates step
    marking is eligible and the sojourn time exceeds `m_L4SMarkThreshold`. When
    `m_StepInPackets` is true, the instantaneous L4S queue length is used
    instead of sojourn time. This routine never drops packets.

  * ``DualPi2QueueDisc::Scheduler()``: This method implements a credit-based WRR
    to select which internal queue to serve. The credit is initialised as
    `m_mtu` * (`m_wClassic` - `m_wL4S`); a negative credit favours the L4S queue
    and a positive credit favours the Classic queue. After each dequeue the
    credit is adjusted by the served packet's size weighted by the opposing
    queue's weight, ensuring the Classic queue receives approximately
    `m_wClassic`% of the total service rate.

  * ``DualPi2QueueDisc::DoDequeue()``: This routine retrieves a packet from the
    ``DualPi2QueueDisc::Scheduler()`` and applies AQM signaling before
    forwarding it. ``DualPi2QueueDisc::MustDrop()`` is evaluated first for all
    packets; if it returns true, the packet is dropped with `OVERLOAD_DROP` and
    the next packet is selected. For L4S packets that pass
    ``DualPi2QueueDisc::MustDrop()``, ``DualPi2QueueDisc::StepAqm()`` is then
    called to apply step-threshold CE marking.


* class :cpp:class:`DualPi2StepTag`: This class implements a custom ns-3 Tag
  used to carry packet-specific state across the queue disc; it stores a boolean
  flag determined during enqueuing to indicate whether the step AQM should be
  applied at the dequeue stage.

Attributes
==========

The :cpp:class:`DualPi2QueueDisc` class holds the following attributes:

* ``Mtu:`` The device MTU in bytes. If set to 0, it is automatically configured
  from the attached NetDevice
* ``A:`` The integral gain factor (alpha) for the PI2 controller. The default
  value is 0.16
* ``B:`` The proportional gain factor (beta) for the PI2 controller. The default
  value is 3.2
* ``Tupdate:`` The sampling interval for the PI2 probability update timer. The
  default value is 16 ms
* ``QueueLimit:`` The total maximum size of the queue disc in bytes.
* ``Target:`` The reference queuing delay target for the Classic queue. The
  default value is 15 ms
* ``L4SMarkThreshold:`` The threshold for the L4S step-AQM marking function. The
  default value is 1 ms
* ``MinQLenStep:`` The minimum L4S queue length (in packets) required to allow
  the step-AQM to trigger. The default value is 0
* ``StepInPackets:`` Whether to apply step function based on queue length in
  packets (true) instead of delay (false). The default value is false
* ``DropEarly:`` Whether to drop at enqueue (true) or dequeue (false). The
  default value is false
* ``DropOverload:`` Whether to drop on overload (true) or overflow (false). The
  default value is true
* ``K:`` The coupling factor (k) between the L4S and Classic queues. The default
  value is 2
* ``ClassicWeight:`` The percentage of service rate (0-100) guaranteed to the
  Classic queue by the WRR scheduler. The default value is 10
* ``StartTime:`` Time to start the queue disc. The default value is 0 s

All the default values follow the default on the Linux implementation.

Traces
======

The :cpp:class:`DualPi2QueueDisc` class provides the following trace sources:

* ``ProbL``: L4S mark probability (p_L).
* ``ProbC``: Classic drop/mark probability (p_C).
* ``ClassicSojournTime``: Sojourn time of the last packet dequeued from the
  Classic queue.
* ``L4sSojournTime``: Sojourn time of the last packet dequeued from the L4S
  queue.


Examples
========

``dualpi2-example.cc``: This example demonstrates the coexistence of a DCTCP
flow (L4S queue) and a TCP Cubic flow (Classic queue) sharing a bottleneck link
managed by the DualPI2 queue disc. It measures per-flow throughput, congestion
window, and queue sojourn times. By default, the simulation is configured with a
bottleneck bandwidth of 40 Mbps, a base RTT of 50 ms, and runs for 20 seconds.
However, these parameters (along with the L4S marking threshold and output
directory) can be easily adjusted via command-line arguments to explore
different network conditions.

Validation
==========

The DualPI2 model is tested using :cpp:class:`DualPi2QueueDiscTestSuite`
class defined in ``src/traffic-control/test/dualpi2-queue-disc-test-suite.cc``.
The suite includes the following test cases:

* Test 1: packets with ECN codepoints Not-ECT and ECT(0) are enqueued into the
  Classic queue; packets with ECT(1) and CE are enqueued into the L4S queue.
* Test 2: the ``DualPi2StepTag`` attached at enqueue controls step AQM marking
  at dequeue; a packet enqueued below ``MinQLenStep`` is not marked even if its
  sojourn time exceeds ``L4SMarkThreshold``. A packet enqueued at or above
  ``MinQLenStep`` is marked if it exceeds ``L4SMarkThreshold``.
* Test 3: ``StepAqm()`` marks packets based on sojourn time when
  ``StepInPackets`` is false; a packet dequeued with sojourn time below
  ``L4SMarkThreshold`` is not marked, while one dequeued sojourn time above is
  marked.
* Test 4: with ``ClassicWeight`` set to 10, the fraction of packets served from
  the Classic queue over 100 dequeue operations converges to approximately 10%.
* Test 5: when the queue reaches ``QueueLimit``, the next packet is dropped with
  reason ``FORCED_DROP`` and the queue size does not increase.
* Test 6: when ``m_baseProb * k > 1.0`` and ``DropOverload`` is true, both
  Classic ECT(0) and L4S ECT(1) packets are dropped with ``OVERLOAD_DROP``
  instead of being marked.
* Test 7: the ``DropEarly`` attribute controls when packets are dropped under
  overload conditions; if true, packets are dropped at enqueue and
  ``OVERLOAD_DROP`` is incremented immediately; if false, packets are enqueued
  successfully and dropped later during dequeue.
* Test 8: with no traffic the PI2 controller holds the probability at zero; when
  a burst arrives, ``m_baseProb`` ramps deterministically and, after three
  update intervals, the traced ``p_C`` and ``p_L`` match their closed-form
  values (``p_L = m_baseProb * k`` and ``p_C = m_baseProb * m_baseProb``).
* Test 9: with ``m_baseProb`` driven into the non-overload region, the fraction
  of packets marked with ``PROBABILISTIC_CLASSIC_MARK`` and
  ``PROBABILISTIC_L4S_MARK`` over many dequeues matches the traced ``p_C`` and
  ``p_L``, confirming that ``MustDrop()`` applies the computed probability.

The test suite can be run using the following commands:

.. sourcecode:: bash

   $ ./ns3 configure --enable-examples --enable-tests
   $ ./ns3 build
   $ ./test.py -s dualpi2-queue-disc


Acknowledgments
===============

This work has been performed within the framework of the FAPESP Engineering
Research Center (ERC) Program under FAPESP grant agreement #2021/00199-8
(SMARTNESS).


References
==========
.. [RFC9332] K. De Schepper, B. Briscoe, and G. White, "Dual-Queue Coupled
             Active Queue Management (AQM) for Low Latency, Low Loss, and
             Scalable Throughput (L4S)", RFC 9332, January 2023,
             <https://www.rfc-editor.org/rfc/rfc9332.html>.
.. [Netdev0x13] B. Briscoe, K. De Schepper, and G. White, "DualPI2: A Dual-Queue
                Coupled PI2 AQM for L4S", Linux Plumbers Conference 2021,
                <https://www.bobbriscoe.net/projects/latency/dualpi2_netdev0x13.pdf>.
.. [SchDualPI2] L4S Team, "L4S Linux Kernel Implementation,"
                https://github.com/L4STeam/linux, 2025, branch: l4steam-6.12.y.
