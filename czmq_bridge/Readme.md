Microblx ZeroMQ Bridge
=========

Dependencies
-----------

It requires 

* CZMQ (git clone git://github.com/zeromq/czmq)
* microblocks


Installation
--------------


* Set up the UBX_ROOT environment variable
```sh
export UBX_ROOT=<path-to-microblx>
```
* Then, download the source in your favorite directory by typing
```sh
git clone <this-prepository-url.git>
mkdir build && cd build
cmake ..
make
```

Quickstart
-----------

```
cd <path_to_ubx_zmq_bridge>
./run_test.sh
```



Use the web interface (http://localhost:8888/) to inspect and start the example.


Design
--------

TBD

TODO
---



License
---

BSD

Authors
-----

Sebastian Blumenthal

Johan Philips
