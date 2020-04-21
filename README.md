# TraceVizPintool
Intel pintool to trace a Windows binary's execution path and create a dot file representation

What is TraceVizPintool?
------------------------

TraceVizPintool is an Intel pintool for tracing the execution flow of Windows binaries. The output is a file in the dot language, suitable for rendering with Graphviz graph visualization software, such a xdot. A trace log file is also generated.

Here is some example output: FIXME FIXME FIXME.

You can learn more about Intel Pintools at https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool.

.You can learn more about Graphviz graph visualization software at https://www.graphviz.org/

Release history
---------------

??/??/2020 version .01. Initial release.

How to run TraceVizPintool
--------------------------

You will need to install Intel Pintools. Then clone this repo.

To run TraceVizPintool:

<path to intel pintools folder>\ia32\bin\pin.exe -t <path to repo>\Debug\TraceVizPintool.dll -- <path to executable to be traced>

By default, the dot output is in traceviz.gv and the log output is in traceviz.log.

How to compile TraceVizPintool
------------------------------

To compile TraceVizPintool, you will need everything required to run TraceVizPintool, plus Microsoft Visual Studio Community 2019 with the Visual C++ desktop product installed.

I suggest installing the root of the repo in <path to intel pintools folder>\source\tools. Then open up the solution file and compile. If you install it somewhere else, you will have to edit the project file to account for the new include and linker paths.

Known Issues
------------

TraceVizPintool has only been tested under x86. So, it can only run with the 32-bit version of pin.exe and can only trace 32-bit binaries.

License and Copyright
---------------------

TraceVizPintools is copyright by P.S. Powledge and licensed under the MIT license.
