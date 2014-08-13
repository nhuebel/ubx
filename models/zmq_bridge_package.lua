return pkg
{
  name="ubx_zmq_bridge",
  path="../",
      
  dependencies = {
 --   { name="ros_bridge", type="cmake" },
  },
  
  
  blocks = {
    { name="zmq_sender", file="examples/zmq_sender.lua", src_dir="src" },
    { name="zmq_receiver", file="examples/zmq_receiver.lua", src_dir="src" },
  },
  
  libraries = {
    { name="zmqsenderlib", blocks={"zmq_sender"} },
    { name="zmqreceiverlib", blocks={"zmq_receiver"} },
  },  
}
