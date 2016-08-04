# Tutorial: How to use the Zyre-UBX bridge


## About
This bridge enables you to link a microblx application to other (possibly remote) applications using Zyre. 

## Prerequisites
Please note that the full functionality requires ROS. Installtion instructions can be found [here](http://wiki.ros.org/ROS/Installation).

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

### Install Zyre:

TODO

Installing and building zyre

### Clone the ubx repository and build the zyre bridge:
```sh
~/workspace$ git clone https://github.com/maccradar/ubx.git
~/workspace$ cd ubx/zyre_bridge
~/workspace/ubx/zyre_bridge$ mkdir build
~/workspace/ubx/zyre_bridge$ cd build
~/workspace/ubx/zyre_bridge/build$ cmake ..
~/workspace/ubx/zyre_bridge/build$ make
~/workspace$ cd ..
```
Use the web interface (http://localhost:8888/) to inspect and start the example.

## Run an example
This assumes that you have built the bridge as described above.
* Open four terminals. 
* In the first run a roscore (only to avoid errors due to ROS related services)
```sh
roscore
```
* In the second run a bridge with a different configuration (see in the .usc file). This one has to be started first if gossip is used.
```sh
./run_zyre_bridge_test.sh
start_all()
```
* In the third run the basic bridge 
```sh
./run_zyre_bridge.sh
start_all()
```
You should see that the two RSGs announce their root nodes to each other.

* In the fourth run a component that sends different messages and waites for the corresponding replies.
```sh
cd test
./test_send_msg
```
You should see that the component sends some update, query, and function block messages to the local RSG. It also waits for the reply and displays it. You can use this component as a template for your own development.



dot -Tps rsg_dump_swm_2016-08-04_17-02-23.gv -o test.ps

## License

The microblx software in this tutorial is licensed under BSD, CZMQ is open source under MPL v2 license, ØMQ is free software licensed under the LGPL.

## Authors

Nico Huebel & Sebastian Blumenthal
