return pkg
{
  name="czmq_ppworker_bridge",
  path="../",
      
  dependencies = {
 --   { name="ros_bridge", type="cmake" },
  },
  
  
  blocks = {
    { name="czmq_ppworker", file="czmq_ppworker.blx", src_dir="src" },
  },
  
  modules = {
    { name="czmqppworkerlib", blocks={"czmq_ppworker"} },
  },  
}
