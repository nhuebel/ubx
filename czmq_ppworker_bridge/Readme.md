Tutorial: Reliable request-reply with heartbeating in the CZMQ-UBX bridge (aka Paranoid Pirate Bridge)
==

About
--
In this tutorial the CZMQ-UBX bridge is extended with reliable request-reply communication using a heartbeating pattern. The microblx application in this example resembles a worker process waiting for a computation task sent by the server queue. These tasks are requested by a client process connected to the server queue.
The example is based on the Paranoid Pirate Pattern described in the ZGuide of ØMQ (found [here](http://zguide.zeromq.org/page:all#Robust-Reliable-Queuing-Paranoid-Pirate-Pattern)).

More information on CZMQ and ØMQ itself can be found [here](http://zguide.zeromq.org/page:all) and [here](http://czmq.zeromq.org/manual:_start). 
Prerequisites
--
### Install ubx dependencies: luajit, libluajit-5.1-dev, clang++, clang
```sh
~/workspace$ sudo apt-get install luajit, libluajit-5.1-dev, clang
```
### Install ubx and follow instructions in the [quickstart](http://ubxteam.github.io/quickstart/).

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

### Install automake:
for being able to run the autogen and configure shell scripts
```sh
~/workspace$ apt-get install automake
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

After building the code, you can run an example configuration in the root directory. In order to test the Paranoid Pirate Pattern, at least one client process and one server queue have to be started as well. Consult the example in the [ZGuide](http://zguide.zeromq.org/page:all#Robust-Reliable-Queuing-Paranoid-Pirate-Pattern) for more information.
Running the script, as shown below, will start a single worker process which tries to connect to a server queue.

The behaviour of the worker process is as follows:
- __worker__ sends _ready_ signal to __queue__
- __worker__ sends _heartbeat_ signal to __queue__
- __worker__ checks if it has received a _heartbeat_ or a _request_ from _queue_, if tries 2 more times with another _heartbeat_ signal
- if __worker__ still has not received any message from __queue__ it destroys the current sockets and tries to reconnect of an increasing time delay (exponential backoff)

The behaviour of the server queue is as follows:
- __server__ waits for requests from __client__
- __server__ sends heartbeats to alive __workers__ (of which it received a _ready_ or _heartbeat_ signal)
- __server__ sends keeps track of alive __workers__ and dispatches requests from __clients__ to __workers__ (round robin)

The behaviour of the client process is as follows:
- __client__ connects to __server__
- __client__ sends request to __server__ and waits for reply
- if no reply is received within a certain time, __client__ destroys the socket and tries to reconnect
- if after three attempts no reply has been received, __client__ abandons the connections.

### Instructions:
```sh
~/workspace$ git clone https://github.com/maccradar/ubx.git
~/workspace$ cd ubx/czmq_bridge
~/workspace/ubx/czmq_bridge$ mkdir build
~/workspace/ubx/czmq_bridge$ cd build
~/workspace/ubx/czmq_bridge/build$ cmake ..
~/workspace/ubx/czmq_bridge/build$ make
~/workspace$ cd ..
~/workspace$ ./run_czmq_ppworker_bridge.sh
```
Use the web interface (http://localhost:8888/) to inspect and start the example.

### Reliable request reply with heartbeating in CZMQ
![Paranoid Pirate Pattern](https://github.com/imatix/zguide/raw/master/images/fig49.png "Paranoid Pirate Pattern")

This image is (c) 2014 iMatix Corporation, licensed under [cc-by-sa 3.0](http://creativecommons.org/licenses/by-sa/3.0/).

License
---

The microblx software in this tutorial is licensed under BSD, CZMQ is open source under MPL v2 license, ØMQ is free software licensed under the LGPL.

Authors
-----

Johan Philips
