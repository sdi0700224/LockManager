# How To

## Project description

Bank(main) creates of copy of source file so original one is not changed. Then with Server it initiates CS and then creates clients.
Client types are created at the same time (readers and writters) with two seperate processes. Then all Clients are waited. Then sems
are destroyed and CS is cleared.

Locking policy is segment locking. When a writer is inside a segment, no one else can enter to this specific segment. Other segments
can be accessed by another writer or multiple readers. If reader(s) is inside segment and a writer comes no new readers are accepted
until writter enters.

Locking mechanism is SegmentLock class, practically a read-writter lock. Implemented a Semaphore class, that uses posix sems with that
provides interupt handling on waiting, throws runtime exception if wait fails and provides an easier Api. I avoided to use contructors
in classes that their instances will be stored inside shared memmory, because automated C++ calls are not working there. If there is a
contructor in these classes, it is only for abstaction reasons, not for this app.

As expected this app is fast for big reader load and small writer load and active time. Also it is faster to implement, with more clear
code and less error prone that a record level locking policy. Of course it does not provide the same granularity but I think it is a
decent way to access small datatypes like integers.

As always coded cleanly, tried to keep some coding standars regarding naming, project architecture and reusability, tested with valgrid 
for leaks and errors.

## Makefile

Added Dubug command to use valgrid
Clean removes files