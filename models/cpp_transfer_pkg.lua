return pkg
{
  name="cpp_transfer",
  path="../",
      
  dependencies = {
-- no dependencies...
  },
  
  types = {
-- define the data type you want to share, e.g. a struct cpp_data
    { name="cpp_data", dir="types" },
  },
  
  blocks = {
-- define the blocks of your package, e.g. a sender and a receiver block
    { name="cpp_sender", file="cpp_sender_block.lua", src_dir="src" },
    { name="cpp_receiver", file="cpp_receiver_block.lua", src_dir="src" },
  },
  
  libraries = {
-- define the libraries. Preferably 1 library per block
    { name="cpp_sender", blocks={"cpp_sender"} },
    { name="cpp_receiver", blocks={"cpp_receiver"} },
  },
}
