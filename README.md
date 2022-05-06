# MaekawasAlgo
Mutual exclusion on distributed file system using Maekawa's Algorithm with mutex at granularity of files.

Implemented Maekawas algorithm for mutual exclusion over distributed file system at granularity of files.
Optimisation of the algorithm is used to prevent deadlocks.

Used messages -
Request
Reply
Inquire
Yield
Failed

To create client and server binaries:
make

To run binaries:
./server
./client <Client id> <port no>

To remove binaries:
make clean

Note: Quorums have been hardcoded to run on UTD DC machines.
Please run servers on dc01, dc07 and dc08 machines.
Please  run clients (1,2,3,4,5) on dc02, dc03, dc04, dc05, dc06 machines respectively.

FS1, FS2, FS3 are the three files systems used for replication.
  
Each quorum has 3 memebers including itself.
  
Implementation is done for 5 clients with each client containing itself, (clientid + 1)%5, (clientid + 2)%5 in its quorom.
