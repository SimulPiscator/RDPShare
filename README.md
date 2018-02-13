# RDPShare
## Windows Desktop Sharing remote control interface

Windows Desktop Sharing (WDS) allows remote interaction with an existing user session.
Typically this requires extra programming effort on the server and client side, and is limited to clients running Windows.
Exploiting the fact that WDS is based on the Windows Remote Assistance (RA) protocol,
RDPShare lets you connect to a WDS session from a machine running the FreeRDP xfreerdp client program.

RDPShare is a Win32 command line program supposed to be run from within an existing user session.
In a typical scenario, a Windows machine would be attached to a projector,
and you would like to control its output from a mobile computer through a wireless network connection.
You might configure the Windows machine to automatically log on the desired user on startup.
Using Windows Task Scheduler to create a task that starts the RDPShare executable whenever that user logs on,
you will be able to control the Windows machine's desktop from a Linux laptop.

By default, RDPShare runs a combined HTTP and control server on port 8080, and a WDS session on port 3390.
Through the control server, the WDS session may be enabled and disabled, and WDS connection information may be retrieved,
which may then be fed to xfreedrp on the command line.
When accessing the control port through a web server, status information is displayed,
and a simple client connection script is offered for download.

## Usage

<pre>RDPShare [http &lt;port&gt;] [rdp &lt;port&gt;] [screen {0..n}]</pre>

Defaults are: http 8080 rdp 3390 screen 0

The "screen" option allows choosing which part of the Windows desktop is shared.
For "screen 0", the entire Windows desktop is shared; for arguments greater 0, only the area of the respective display is shared.

The following requests are recognized when when sent to the HTTP port:
* start: start WDS session
* stop: stop WDS session
* quit: exit RDAShare process
* get_ConnectionString: get WDS connection string
* get_RASessionID: get Remote Assistance session ID
* get_SharedRect: get shared rect
