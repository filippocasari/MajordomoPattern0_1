# MajordomoPattern vv. 0_1

This is one of the most popular patterns of ZMQ. 

# Components
  
The architecture of this pattern is based on 3 blocks:
   1) Broker: who is in the middle of the information flow. Here the workers can subscribe and clients can send their request. It's up to the broker to provide a simple forward
      and send them to the workers. 
   2) Worker: who takes the data generated by its own or by sensor or other sources and send them to the client that wants this data.
   3) Client: who wants the data provided by the worker.
 
 
# Structure


