# TraceVizPintool
Intel pintool to trace a Windows binary's execution path and create a graphical representation of the library calls, image loads, and instruction source segments.

## What is TraceVizPintool?

TraceVizPintool is a tool to trace a Windows binary's execution path and create a graphical representation of the called library functions, loaded images, and executed instruction segments.

Here's an example. We're running TraceVizPintool on a packed malware sample from the book [Practical Malware Analysis](https://nostarch.com/malware) by Michael Sikorski and Andrew Honig. The specific sample is lab18-01.exe, which uses a modified version of the UPX packer to unpack and execute the code. This is the output:

![TraceVizPintool output - overview](https://github.com/PollyP/TraceVizPintool/blob/master/examples/lab18-01-overview.png)

The gray boxes are the image loads. The pink boxes is the unpacking code, executing out of one segment. Here's a close-up:

![TraceVizPintool output - unpacker detail](https://github.com/PollyP/TraceVizPintool/blob/master/examples/lab18-01-detail-01.png)

Now we switch to yellow boxes; this means we are now executing instructions from a different section. This is a "section hop," and in this case it means we have finished unpacking and are now executing the payload. Here's a close-up:

![TraceVizPintool output - payload detail](https://github.com/PollyP/TraceVizPintool/blob/master/examples/lab18-01-detail-02.png)

## What do I need to run TraceVizPintool?

TraceVizPintool is an Intel pintool, so you'll need Intel's Windows pintools to run it. You can learn more about Intel Pintools at https://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool.

TraceVizPintools produces several artifacts: a file in the dot language, suitable for rendering with Graphviz graph visualization software (such as xdot); a comma-separated text file, suitable for importing into a spreadsheet program; and a trace log file.

You can learn more about Graphviz graph visualization software at https://www.graphviz.org/

## How to run TraceVizPintool

You will need to install Intel Windows Pintools, available at https://software.intel.com/en-us/articles/pin-a-binary-instrumentation-tool-downloads. Then download or clone this repo.

At the command line type:

```<path to intel pintools folder>\ia32\bin\pin.exe -t <path to repo>\Debug\TraceVizPintool.dll -- <path to executable to be traced>```

By default, the dot output is in traceviz.gv, the csv file is in traceviz.csv, and the log output is in traceviz.log.

## How to compile TraceVizPintool

To compile TraceVizPintool, you will need everything required to run TraceVizPintool, plus Microsoft Visual Studio Community 2019 with the Visual C++ desktop product installed.

I suggest installing the root of the repo in <path to intel pintools folder>\source\tools. Then open up the solution file and compile. If you install it somewhere else, you will have to edit the project file to account for the new include and linker paths.

## Known Issues

TraceVizPintool has only been tested under x86. So, it can only run with the 32-bit version of pin.exe and can only trace 32-bit binaries.

Sometimes dot client tools run out of memory trying to render the graphs with lots of library calls. In that case, importing the CSV output into a spreadsheet and analyzing from there is the better bet.

By default, TraceVizPintools just traces the first five threads in a program.

## Release history

04/24/2020 version 00.01. Initial release.

## Test Environment

TraceVizPintool has been tested with version 3.11 of the Intel Pintools, running on 64-bit Windows 8.1. It has been compiled with Microsoft Visual Studio Community 2019.

For bug reports, use the Github issue tracker.

## License and Copyright

TraceVizPintools is copyright by P.S. Powledge and licensed under the MIT license. The terms of the license are in the source.
