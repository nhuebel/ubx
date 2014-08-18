Tutorial: How to use the CZMQ-UBX bridge
========================

About
---
This bridge enables you to link a microblx application to other (possibly remote) applications using CZMQ sockets. CZMQ is a high-level C binding for ØMQ, a networking library with _sockets on steroids_.

In this tutorial PUB-SUB sockets are used to demonstrate the communcation between two microblx applications (a publisher and a subscriber).
Configuration parameters allow you to change the protocol (e.g. tcp://, ipc://, pgm://, inproc://) and the address and port, if relevant.

More information on CZMQ and ØMQ itself can be found at http://zguide.zeromq.org/page:all and http://czmq.zeromq.org/manual:_start 
Prerequisites
-------------
### Install ubx dependencies: luajit, libluajit-5.1-dev, clang++, clang
```sh
~/workspace$ sudo apt-get install luajit, libluajit-5.1-dev, clang
```
### Install ubx and follow instructions at <http://ubxteam.github.io/quickstart/>

```sh
~/workspace$ git clone https://github.com/UbxTeam/microblx.git
~/workspace$ cd microblx
```

### Install ubx cmake:

```sh
~/workspace$ git clone https://github.com/haianos/microblx_cmake.git
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

Building & running the CZMQ bridge example
---

After building the code, you can run an example configuration in the root directory.
This will connect at the publisher side a random number generator block with the sender block and at the subscriber side the receiver block with
a hexdump block. Sender and receiver are connected through the configurable CZMQ PUB-SUB sockets.

### Clone the ubx repository:
```sh
~/workspace$ git clone https://github.com/maccradar/ubx.git
~/workspace$ cd ubx/czmq_bridge
~/workspace/ubx/czmq_bridge$ mkdir build
~/workspace/ubx/czmq_bridge$ cd build
~/workspace/ubx/czmq_bridge/build$ cmake ..
~/workspace/ubx/czmq_bridge/build$ make
~/workspace$ cd ..
~/workspace$ ./run_czmq_bridge.sh
```
Use the web interface (http://localhost:8888/) to inspect and start the example.

### PUB-SUB sockets in CZMQ
![PUB-SUB](https://github.com/imatix/zguide/raw/master/images/fig4.png "PUB-SUB")

This image is (c) 2014 iMatix Corporation, licensed under [cc-by-sa 3.0](http://creativecommons.org/licenses/by-sa/3.0/).

License
---

The microblx software in this tutorial is licensed under BSD, CZMQ is open source under MPL v2 license, ØMQ is free software licensed under the LGPL.

Authors
-----

Sebastian Blumenthal & Johan Philips
