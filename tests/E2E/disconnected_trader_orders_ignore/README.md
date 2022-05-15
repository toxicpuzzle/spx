This testcase uses a special trader binary that only
writes messages to the pipe and does not signal the parent,
to demonstrate that the parent only reads from connected children
that is capable of signalling itself.