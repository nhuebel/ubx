Tutorial: How to use the Zyre-UBX bridge
========================

About
---
This bridge enables you to link a microblx application to other (possibly remote) applications using Zyre sockets. 

Prerequisites
-------------
### Install ubx dependencies: luajit, libluajit-5.1-dev, clang++, clang
```sh
~/workspace$ sudo apt-get install luajit, libluajit-5.1-dev, clang
```
### Install ubx and follow instructions in the [quickstart](http://ubxteam.github.io/quickstart/).

```sh
~/workspace$ git clone https://github.com/UbxTeam/microblx.git
~/workspace$ cd microblx
```

### Set some environment variables and run the env.sh script (e.g. in your .bashrc):

```sh
export UBX_ROOT=~/workspace/microblx
export UBX_MODULES=~/workspace/install/lib/microblx
source $UBX_ROOT/env.sh
```

### Install CZMQ:

```sh
~/workspace$ git clone git://github.com/zeromq/czmq
~/workspace$ cd czmq
~/workspace/czmq$ ./autogen.sh
~/workspace/czmq$ ./configure
~/workspace/czmq$ make
~/workspace/czmq$ sudo make install
```

## Install Zyre

TBD

Building & running the Zyre bridge example
---


### Clone the ubx repository:
```sh
~/workspace$ git clone https://github.com/maccradar/ubx.git
~/workspace$ cd ubx/zyre_bridge
~/workspace/ubx/zyre_bridge$ mkdir build
~/workspace/ubx/zyre_bridge$ cd build
~/workspace/ubx/zyre_bridge/build$ cmake ..
~/workspace/ubx/zyre_bridge/build$ make
~/workspace$ cd ..
~/workspace$ ./run_zyre_bridge.sh
```
Use the web interface (http://localhost:8888/) to inspect and start the example.

License
---

The microblx software in this tutorial is licensed under BSD, CZMQ is open source under MPL v2 license, ØMQ is free software licensed under the LGPL.

Authors
-----

Nico Hübel & Sebastian Blumenthal
