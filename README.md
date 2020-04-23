# TraceVizPintool
Intel pintool to trace a Windows binary's execution path and create a dot file and comma separated file representation

What is TraceVizPintool?
------------------------

TraceVizPintool is an Intel pintool for tracing the execution flow of Windows binaries. The output is (1) a file in the dot language, suitable for rendering with Graphviz graph visualization software, such a xdot, and (2) a comma-separated text file, suitable for importing into a spreadsheet. A trace log file is also generated.

You can learn more about Intel Pintools at https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool. You can learn more about Graphviz graph visualization software at https://www.graphviz.org/

Examples
--------

All these examples have been run against the malware samples used in chapter 18 of "Practical Malware Analysis." The book is available at https://nostarch.com/malware, and the lab samples are available at https://github.com/mikesiko/PracticalMalwareAnalysis-Labs. As you may have deduced, the samples are malware and you should run them in a safe testing environment. (And if you don't know how to do that, this book is a great place to start.)

Release history
---------------

??/??/2020 version .01. Initial release.

How to run TraceVizPintool
--------------------------

You will need to install Intel Pintools. Then clone this repo.

To run TraceVizPintool:

```<path to intel pintools folder>\ia32\bin\pin.exe -t <path to repo>\Debug\TraceVizPintool.dll -- <path to executable to be traced>```

By default, the dot output is in traceviz.gv, the csv files is in traceviz.csv, and the log output is in traceviz.log.

How to compile TraceVizPintool
------------------------------

To compile TraceVizPintool, you will need everything required to run TraceVizPintool, plus Microsoft Visual Studio Community 2019 with the Visual C++ desktop product installed.

I suggest installing the root of the repo in <path to intel pintools folder>\source\tools. Then open up the solution file and compile. If you install it somewhere else, you will have to edit the project file to account for the new include and linker paths.

Known Issues
------------

TraceVizPintool has only been tested under x86. So, it can only run with the 32-bit version of pin.exe and can only trace 32-bit binaries.

I've noticed that sometimes the Dot tools have a problem with the graphs generated from samples with lots of library calls. In that case, importing the CSV output into a spreadsheet and analyzing from there is a better bet.

By default, TraceVizPintools just traces the first five threads in a program.

Test Environment
----------------

TraceVizPintool has been tested with version 3.11 of the Intel Pintools, running on 64-bit Windows 8.1. It has been compiled with Microsoft Visual Studio Community 2019.

Bug Reports
-----------

Bug reports are appreciated. PRs are welcome.

License and Copyright
---------------------

TraceVizPintools is copyright by P.S. Powledge and licensed under the MIT license. The terms of the license are in the source.
