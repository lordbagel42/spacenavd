spacenavd
=========

![GNU/Linux build status](https://github.com/FreeSpacenav/spacenavd/actions/workflows/build_gnulinux.yml/badge.svg)
![FreeBSD build status](https://github.com/FreeSpacenav/spacenavd/actions/workflows/build_freebsd.yml/badge.svg)
![MacOS X build status](https://github.com/FreeSpacenav/spacenavd/actions/workflows/build_macosx.yml/badge.svg)

About
-----
Spacenavd is a free software user-space driver (daemon), for 6-dof input
devices, like 3Dconnexion's space-mice.

This version includes enhanced support for the **SpaceMouse Enterprise**,
a built-in **WebSocket server** for browser applications, and a
**device simulator** for testing.

License
-------
Copyright (C) 2007-2025 John Tsiombikas <nuclear@mutantstargoat.com>
Copyright (C) 2025 Jules

This program is free software. Feel free to copy, modify and/or redistribute it
under the terms of the GNU General Public License version 3, or at your option,
any later version published by the Free Software Foundation. See COPYING for
details.

Dependencies
------------
In order to compile the spacenavd daemon, you'll need the following:
 - GNU C Compiler
 - GNU make
 - OpenSSL (libssl-dev) - **New requirement for WebSockets**
 - Xlib (libX11, optional)

For the configuration GUI and simulator:
 - Python 3
 - websockets (python library: `pip install websockets`)
 - tkinter (usually included with Python)

Installation
------------
1. Run `./configure`
2. Run `make`
3. Run `sudo make install`

Running spacenavd
-----------------
Start the daemon with:
```bash
sudo spacenavd -d -v
```
The WebSocket server will start on port 8000 by default.

SpaceMouse Enterprise Simulator
-------------------------------
If no physical device is detected, spacenavd will automatically start a
simulated device listening on UDP port 9999.
You can send simulated events using the provided tool:
```bash
python3 contrib/enterprise_tools/enterprise_sim.py
```

Configuration GUI
----------------
A new native configuration GUI is provided in:
```bash
python3 contrib/enterprise_tools/spnavcfg_native.py
```
This GUI uses WebSockets to communicate with the running daemon.

Troubleshooting
---------------
Check `/var/log/spnavd.log` for output and error messages.
