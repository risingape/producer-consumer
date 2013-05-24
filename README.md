== producer-consumer

   The classic producer-consumer problem implemented using POSIX IPC and double
   buffering in shared memory. Both, the producer and the consumer process got 
   access to two buffers in shared memory. While the producer writes data into
   one buffer, the consumer is able to read the second buffer. This avoids
   stalling any of the processes.

   Each buffer is guarded by a read and write semaphore signalling whether it
   is allowed to read ore write to a buffer. Four semaphores are necessary to
   ensure only valid data is written or read. 
