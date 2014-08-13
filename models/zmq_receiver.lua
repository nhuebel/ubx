return block
{
      name="zmq_receiver",
      meta_data="Generic cblock that recieves a bytestream (unsigend char) as a ZMQ message.",
      port_cache=true,

      configurations= {
	{ name="connection_spec", type_name = "char", doc="Connection string that defines the ZMQ type of connection. E.g. tcp://localhost:11411"},
      },

      ports = {
	 { name="zmq_in", out_type_name="unsigned char", doc="Arbitrary byte stream as recieved by a ZMQ message." },
      },
      
      operations = { start=true, stop=true, step=true },
      
      cpp=true
}
