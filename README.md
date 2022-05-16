# Malcolm Morton
# CSCE 3530.00
# 4/28/22

# Compile:
1. type "make"
2. server will be "tcpserver"
3. client will be "tcpclient"

# Run:
1. type "./tcpserver"
2. type "./tcpclient"

# Clean:
1. type "make clean"

# Test:
1. give ./tcpserver a port number when prompted
2. give ./tcpclient the same port number when prompted

# Remarks:
- I'm using /dev/random file from linux, which may block occassionally at startup until sufficient entropy has been gathered from
- the environmental noise collected from device device drivers meaning it was blocking at the second call in tcpclient
- I removed the blocking call to /dev/random and replaced it with /dev/urandom (line 155) which simply appends pseudorandom bits if it runs out
- These bits of code are located at lines 116-122 in tcpserver.c and lines 73-79 and 152-158 in tcpclient.c (including the comment I made)

# Known Bugs/Issues:
1. No printing to client.out and server.out