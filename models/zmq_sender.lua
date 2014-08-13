return block
{
      name="zmq_sender",
      meta_data="Generic cblock that sends a bytestream (unsigend char) as a ZMQ message.",
      port_cache=true,

      configurations = {
	{ name="connection_spec", type_name = "char", doc="Connection string that defines the ZMQ type of connection. E.g. tcp://*:11411 "},
      },

      ports = {
	 { name="zmq_out", in_type_name="unsigned char", doc="Arbitrary byte stream to be send as a ZMQ message." },
      },
      
      operations = { start=true, stop=true, step=true },
      
      cpp=true
}
