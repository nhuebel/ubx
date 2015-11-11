#   Request-reply client in Python
#   Connects REQ socket to tcp://localhost:5559
#   Sends "Hello" to server, expects "World" back
#
import zmq

#  Prepare our context and sockets
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:22422")

#  Do 10 requests, waiting each time for a response
for request in range(1,11):
    socket.send(b"Hello")
    message = socket.recv()
    print("Received reply %s [%s]" % (request, message))
    socket.send(b"World")
    message = socket.recv()
    print("Received reply %s [%s]" % (request, message))
    socket.send(b"-----")
    print("Waiting...")
    message = socket.recv()
    print("Received reply %s [%s]" % (request, message))
