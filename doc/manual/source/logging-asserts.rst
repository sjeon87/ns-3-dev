.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Logging
-------

The |ns3| logging facility can be used to monitor or debug the progress
of simulation programs.  Logging output can be enabled by program statements
in your ``main()`` program or by the use of the ``NS_LOG`` environment variable.

Logging statements are not compiled into ``optimized`` builds of |ns3|.  To use
logging, one must use the ``default`` or ``debug`` build profiles of |ns3|.

The project makes no guarantee about whether logging output will remain
the same over time.  Users are cautioned against building simulation output
frameworks on top of logging code, as the output and the way the output
is enabled may change over time.

Overview
********

|ns3| logging statements are typically used to log various program
execution events, such as the occurrence of simulation events or the
use of a particular function.

For example, this code snippet is from ``TcpSocketBase::EnterCwr()`` and informs the user that
the model is reducing the congestion window and changing state::

  NS_LOG_INFO("Enter CWR recovery mode; set cwnd to " << m_tcb->m_cWnd << ", ssthresh to "
                                                      << m_tcb->m_ssThresh << ", recover to "
                                                      << m_recover);

If logging has been enabled for the ``Ipv4L3Protocol`` component at a severity
of ``INFO`` or above (see below about log severity), the statement
will be printed out; otherwise, it will be suppressed.

The logging implementation is enabled in ``debug`` and ``default``
builds, but disabled in all other build profiles,
so that it does not impact the execution speed of more optimized profiles.

You can try the example program `log-example.cc` in `src/core/example`
with various values for the `NS_LOG` environment variable to see the
effect of the options discussed below.

Enabling Output
***************

There are two ways that users typically control log output.  The
first is by setting the ``NS_LOG`` environment variable; e.g.:

.. sourcecode:: bash

   $ NS_LOG="*" ./ns3 run first

will run the ``first`` tutorial program with all logging output.  (The
specifics of the ``NS_LOG`` format will be discussed below.)

This can be made more granular by selecting individual components:

.. sourcecode:: bash

   $ NS_LOG="Ipv4L3Protocol" ./ns3 run first

The output can be further tailored with prefix options.

The second way to enable logging is to use explicit statements in your
program, such as in the ``first`` tutorial program::

   int
   main(int argc, char *argv[])
   {
     LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
     LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
     ...

(The meaning of ``LOG_LEVEL_INFO``, and other possible values,
will be discussed below.)

``NS_LOG`` Syntax
*****************

The ``NS_LOG`` environment variable contains a list of log components
and options.  Log components are separated by \`:' characters:

.. sourcecode:: bash

   $ NS_LOG="<log-component>:<log-component>..."

Options for each log component are given as flags after
each log component:

.. sourcecode:: bash

   $ NS_LOG="<log-component>=<option>|<option>...:<log-component>..."

Options control the severity and level for that component,
and whether optional information should be included, such as the
simulation time, simulation node, function name, and the symbolic severity.

Log Components
==============

Generally a log component refers to a single source code ``.cc`` file,
and encompasses the entire file.

Some helpers have special methods to enable the logging of all components
in a module, spanning different compilation units, but logically grouped
together, such as the |ns3| wifi code::

   WifiHelper wifiHelper;
   wifiHelper.EnableLogComponents();

The ``NS_LOG`` log component wildcard \`*' will enable all components.

To see what log components are defined, any of these will work:

.. sourcecode:: bash

   $ NS_LOG="print-list" ./ns3 run ...

   $ NS_LOG="foo"  # a token not matching any log-component

The first form will print the name and enabled flags for all log components
which are linked in; try it with ``scratch-simulator``.
The second form prints all registered log components,
then exit with an error.


Severity and Level Options
==========================

Individual messages belong to a single "severity class," set by the macro
creating the message.  In the example above,
``NS_LOG_INFO(..)`` creates the message in the ``LOG_INFO`` severity class.

The following severity classes are defined as ``enum`` constants:

================  =========================================================
Severity Class    Meaning
================  =========================================================
``LOG_NONE``      The default, no logging
``LOG_ERROR``     Serious error messages only
``LOG_WARN``      Warning messages
``LOG_INFO``      Info about the model changing state
``LOG_FUNCTION``  Function tracing
``LOG_LOGIC``     For tracing key decision points or branches in a function
``LOG_DEBUG``     For use in debugging
================  =========================================================

Typically one wants to see messages at a given severity class *and higher*.
This is done by defining inclusive logging "levels":

======================  ===========================================
Level                   Meaning
======================  ===========================================
``LOG_LEVEL_ERROR``     Only ``LOG_ERROR`` severity class messages.
``LOG_LEVEL_WARN``      ``LOG_WARN`` and above.
``LOG_LEVEL_INFO``      ``LOG_INFO`` and above.
``LOG_LEVEL_FUNCTION``  ``LOG_FUNCTION`` and above.
``LOG_LEVEL_LOGIC``     ``LOG_LOGIC`` and above.
``LOG_LEVEL_DEBUG``     ``LOG_DEBUG`` and above.
``LOG_LEVEL_ALL``       All severity classes.
``LOG_ALL``             Synonym for ``LOG_LEVEL_ALL``
======================  ===========================================

The severity class and level options can be given in the ``NS_LOG``
environment variable by these tokens:

============  =================
Class         Level
============  =================
``error``     ``level_error``
``warn``      ``level_warn``
``info``      ``level_info``
``function``  ``level_function``
``logic``     ``level_logic``
``debug``     ``level_debug``
..            | ``level_all``
              | ``all``
              | ``*``
============  =================

Using a severity class token enables log messages at that severity only.
For example, ``NS_LOG="*=warn"`` won't output messages with severity ``error``.
``NS_LOG="*=level_debug"`` will output messages at severity levels
``debug`` and above.

Severity classes and levels can be combined with the \`|' operator:
``NS_LOG="*=level_warn|debug"`` will output messages at severity levels
``error``, ``warn`` and ``debug``, but not ``info``, ``function``, or ``logic``.

The ``NS_LOG`` severity level wildcard \`*' and ``all``
are synonyms for ``level_all``.

For log components merely mentioned in ``NS_LOG``

.. sourcecode:: bash

   $ NS_LOG="<log-component>:..."

the default severity is ``LOG_LEVEL_ALL``.


Prefix Options
==============

A number of prefixes can help identify
where and when a message originated, and at what severity.

The available prefix options (as ``enum`` constants) are

======================  ===========================================
Prefix Symbol           Meaning
======================  ===========================================
``LOG_PREFIX_FUNC``     Prefix the name of the calling function.
``LOG_PREFIX_TIME``     Prefix the simulation time.
``LOG_PREFIX_NODE``     Prefix the node id.
``LOG_PREFIX_LEVEL``    Prefix the severity level.
``LOG_PREFIX_ALL``      Enable all prefixes.
======================  ===========================================

The prefix options are described briefly below.

The options can be given in the ``NS_LOG``
environment variable by these tokens:

================  =========
Token             Alternate
================  =========
``prefix_func``   ``func``
``prefix_time``   ``time``
``prefix_node``   ``node``
``prefix_level``  ``level``
``prefix_all``    | ``all``
                  | ``*``
================  =========

For log components merely mentioned in ``NS_LOG``

.. sourcecode:: bash

   $ NS_LOG="<log-component>:..."

the default prefix options are ``LOG_PREFIX_ALL``.

Severity Prefix
###############

The severity class of a message can be included with the options
``prefix_level`` or ``level``.  For example, this value of ``NS_LOG``
enables logging for all log components (\`*') and all severity
classes (``=all``), and prefixes the message with the severity
class (``|prefix_level``).

.. sourcecode:: bash

   $ NS_LOG="*=all|prefix_level" ./ns3 run scratch-simulator
   Scratch Simulator
   [ERROR] error message
   [WARN] warn message
   [INFO] info message
   [FUNCT] function message
   [LOGIC] logic message
   [DEBUG] debug message

Time Prefix
###########

The simulation time can be included with the options
``prefix_time`` or ``time``.  This prints the simulation time in seconds.

Node Prefix
###########

The simulation node id can be included with the options
``prefix_node`` or ``node``.

Function Prefix
###############

The name of the calling function can be included with the options
``prefix_func`` or ``func``.


``NS_LOG`` Wildcards
====================

The log component wildcard \`*' will enable all components.  To
enable all components at a specific severity level
use ``*=<severity>``.

The severity level option wildcard \`*' is a synonym for ``all``.
This must occur before any \`|' characters separating options.
To enable all severity classes, use ``<log-component>=*``,
or ``<log-component>=*|<options>``.

The option wildcard \`*' or token ``all`` enables all prefix options,
but must occur *after* a \`|' character.  To enable a specific
severity class or level, and all prefixes, use
``<log-component>=<severity>|*``.

The combined option wildcard ``**`` enables all severities and all prefixes;
for example, ``<log-component>=**``.

The uber-wildcard ``***`` enables all severities and all prefixes
for all log components.  These are all equivalent:

.. sourcecode:: bash

   $ NS_LOG="***" ...      $ NS_LOG="*=all|*" ...        $ NS_LOG="*=*|all" ...
   $ NS_LOG="*=**" ...     $ NS_LOG="*=level_all|*" ...  $ NS_LOG="*=*|prefix_all" ...
   $ NS_LOG="*=*|*" ...

Be advised:  even the trivial ``scratch-simulator`` produces over
46K lines of output with ``NS_LOG="***"``!


Adding logging to your code
***************************

For developer guidelines on how and when to insert logging statements
in proposed |ns3| code, see the
`Logging section <https://www.nsnam.org/docs/contributing/html/coding-style.html#logging>`_
of the coding style chapter in the contributing guide.

Controlling timestamp precision
*******************************

Timestamps are printed out in units of seconds.  When used with the default
|ns3| time resolution of nanoseconds, the default timestamp precision is 9
digits, with fixed format, to allow for 9 digits to be consistently printed
to the right of the decimal point.  Example:

::

  +0.000123456s RandomVariableStream:SetAntithetic(0x805040, 0)

When the |ns3| simulation uses higher time resolution such as picoseconds
or femtoseconds, the precision is expanded accordingly; e.g. for picosecond:

::

  +0.000123456789s RandomVariableStream:SetAntithetic(0x805040, 0)

When the |ns3| simulation uses a time resolution lower than microseconds,
the default C++ precision is used.

An example program at ``src/core/examples/sample-log-time-format.cc``
demonstrates how to change the timestamp formatting.

The maximum useful precision is 20 decimal digits, since Time is signed 64
bits.


Asserts
*******

The |ns3| assert facility can be used to validate that invariant conditions
are met during execution. If the condition is not met an error message is given
and the program stops, printing the location of the failed assert.

The assert implementation is enabled in ``debug`` and ``default``
builds, but disabled in all other build profiles to improve execution speed.

Adding asserts to your code
===========================

For developer guidelines on how and when to insert assert (and abort
and fatal-error) statements in proposed |ns3| code, see the
`Asserts section <https://www.nsnam.org/docs/contributing/html/coding-style.html#asserts>`_
of the coding style chapter in the contributing guide.
