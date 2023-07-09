# ml-rga-test

Test repo for Arduino SRS RGA library.

Library code is in ml-rga directory. Will eventually be broken off into separate repo or subrepo 
to allow installation in projects.

Intended to provide non-blocking serial control and parsing of RGA serial data. 
RGA commands are slow; returns take 0.1s to several seconds.

Goal is to use serial reader in a rapidly running loop() to put together reply 
packets as they come in. Packet completeness controlled by 
number of bytes expected in the response. Signal to start reading is setting `packetLength` > 0. 
Availability of completed packet is signaled by `newData == true`.

System is still clunky. Need a nice way to handle polling and moving to next command.

Current idea is to separate writes and reads. Writes will check if a write is already in progress
or if previous result hasn't been read yet and return 0 without running. Allows including writes
in the loop. Reads execute the serial reader and check if newData is available. This could get ugly 
for a series of sequential actions.


