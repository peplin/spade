#!/usr/bin/env python

import zmq
import time
context = zmq.Context()

socket = context.socket(zmq.PAIR)
socket.connect("ipc:///tmp/adder.sock")

while True:
       message = socket.recv()
       print message
       #publisher.send(message)
