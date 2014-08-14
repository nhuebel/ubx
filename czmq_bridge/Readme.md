Tutorial: How to use the CZMQ-UBX bridge
========================

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

If you quickly want to get started, you can build the code and run the example configuration in the root directory.
This will connect at one side a random number generator block with the sender block and at the other side the receiver block with
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

License
---

BSD

Authors
-----

Sebastian Blumenthal & Johan Philips
