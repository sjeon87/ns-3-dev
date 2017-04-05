.. include:: replace.txt
.. highlight:: cpp

PI Square queue disc
--------------------

PI Square or PI2 [Schepper16]_ is a variant of PIE ([Pan13]_, [Pan16]_) whose behavior
is intended to be similar to that of PIE, but with reduced computation.

This chapter describes the PI Square queue disc implementation in |ns3|.

Model Description
*****************

The source code for the PI2 model is located in the directory ``src/traffic-control/model``
and consists of 2 files `pi-square-queue-disc.h` and `pi-square-queue-disc.cc` defining a
PiSquareQueueDisc class. The code was ported to |ns3| by Rohit P. Tahiliani based on the
Linux kernel code of PI2. This model does not support certain features of PIE, including
ECN, autotuning and optional design elements mentioned in RFC 8033.

* class :cpp:class:`PiSquareQueueDisc`: This class implements the main PI2 algorithm:

  * ``PiSquareQueueDisc::DoEnqueue()``: This routine checks whether the queue is full, and if so, drops the packets and records the number of drops due to queue overflow. If queue is not full, this routine calls ``PiSquareQueueDisc::DropEarly()``, and depending on the value returned, the incoming packet is either enqueued or dropped.

  * ``PiSquareQueueDisc::DropEarly()``: The decision to enqueue or drop the packet is taken by invoking this routine, which returns a boolean value; false indicates enqueue and true indicates drop.

  * ``PiSquareQueueDisc::CalculateP()``: This routine is called at a regular interval of `m_tUpdate` and updates the drop probability, which is required by ``PiSquareQueueDisc::DropEarly()``

  * ``PiSquareQueueDisc::DoDequeue()``: This routine calculates the average departure rate which is required for updating the drop probability in ``PiSquareQueueDisc::CalculateP()``

References
==========

.. [Schepper16] De Schepper, K., Bondarenko, O., Tsang, J., & Briscoe, B. (2016, November). PI2: A Linearized AQM for both Classic and Scalable TCP. In Proceedings of the 12th International on Conference on emerging Networking EXperiments and Technologies (pp. 105-119). ACM.`_.

.. [Pan13] Pan, R., Natarajan, P., Piglione, C., Prabhu, M. S., Subramanian, V., Baker, F., & VerSteeg, B. (2013, July). PIE: A lightweight control scheme to address the bufferbloat problem. In High Performance Switching and Routing (HPSR), 2013 IEEE 14th International Conference on (pp. 148-155). IEEE.  Available online at `<https://www.ietf.org/mail-archive/web/iccrg/current/pdfB57AZSheOH.pdf>`_.

.. [Pan16] R. Pan, P. Natarajan, F. Baker, G. White, B. VerSteeg, M.S. Prabhu, C. Piglione, V. Subramanian, Internet-Draft: PIE: A lightweight control scheme to address the bufferbloat problem, April 2016.  Available online at `<https://tools.ietf.org/html/draft-ietf-aqm-pie-07>`_.


Attributes
==========

The key attributes that the PiSquareQueueDisc class holds include the following:

* ``MaxSize:`` The maximum number of bytes or packets the queue can hold.
* ``MeanPktSize:`` Mean packet size in bytes. The default value is 1000 bytes.
* ``Tupdate:`` Time period to calculate drop probability. The default value is 30 ms.
* ``Supdate:`` Start time of the update timer. The default value is 0 ms.
* ``DequeueThreshold:`` Minimum queue size in bytes before dequeue rate is measured. The default value is 10000 bytes.
* ``QueueDelayReference:`` Desired queue delay. The default value is 20 ms.
* ``A:`` Value of alpha. The default value is 0.625.
* ``B:`` Value of beta. The default value is 6.25.

Examples
========

The example for PI2 is `pi-square-example.cc` located in ``src/traffic-control/examples``.  To run the file (the first invocation below shows the available command-line options):

.. sourcecode:: bash

   $ ./ns3 run "pi-square-example" -- --PrintHelp
   $ ./ns3 run "pi-square-example"

Validation
**********

The PI2 model is tested using :cpp:class:`PiSquareQueueDiscTestSuite` class defined in `src/traffic-control/test/pi-square-queue-disc-test-suite.cc`. The suite includes 2 test cases:

* Test Case 1: Basic enqueue/dequeue and attribute setting using the following test scenarios:

  * Scenario 1: simple enqueue/dequeue with defaults, no drops
  * Scenario 2: more data with defaults, unforced drops but no forced drops
  * Scenario 3: same as Scenario 2, but with higher QueueDelayReference
  * Scenario 4: same as Scenario 2, but with lower dequeue rate

* Test Case 2: Dumbbell topology validation test, verifying queue delay bounds under different traffic scenarios:

  * Scenario 1: 5 TCP flows
  * Scenario 2: 50 TCP flows
  * Scenario 3: Mixed TCP and UDP traffic flows

The test suite can be run using the following commands:

.. sourcecode:: bash

  $ ./ns3 configure --enable-examples --enable-tests
  $ ./ns3 build
  $ ./test.py -s=pi-square-queue-disc

or alternatively (to see logging statements in a debug build):

.. sourcecode:: bash

  $ NS_LOG="PiSquareQueueDisc" ./ns3 --run "test-runner --suite=pi-square-queue-disc"
