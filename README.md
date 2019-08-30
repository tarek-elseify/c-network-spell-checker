# c-network-spell-checker
## A multi-threaded networked spell checker<br />
<br />

This C program implements multi-threading to spell check input from different user. The maximum number of people allowed in a connection is equal to the number of threads set. This program is thread safe, as it protects the log file by requiring a lock when writing to it. Since the dictionary is read only, it does not need a lock. This is a simple spell checker that only checks a single word.
<br />
# NOTE
To use this program, you must execute server. This will expost a port (default 10000) to listen on and receive messages. On a seperate machine or terminal, telnet into that port (ie telnet 10000) and you will be connected to the network. This program will log every event. You can specify a different port number by server -p XXX or a different dictionary by -d /path/to/dict.txt, or both server -p 8888 -d /path/to/dict.txt
