#!/usr/bin/env python
# -*- coding: utf-8 -*-

""" 
Expect like class for XTF.
Should be used if pexpect is not available
"""

from   Queue import Queue, Empty
from   subprocess import Popen, PIPE
from   threading import Thread

class xtf_spawn(object):
    """pexpect.spawn like class for XTF"""

    def __init__(self, command, args, timeout=30):

        if not isinstance(args, type([])):
            raise TypeError('The argument, args, must be a list.')

        args.insert(0, command)

        self.io_q = Queue()
        self.proc = Popen(args, stdout = PIPE, stderr = PIPE)
        Thread(target=self._reader_fn, name="reader",
            args=(self.proc.stdout,)).start()

    def _reader_fn(self, stream):
        """Reader Thread"""
        line = stream.readline()
        while line:
            self.io_q.put(line)
            line = stream.readline()

        if not stream.closed:
            stream.close()

    def expect(self, pattern, timeout=-1):
        """Print console output"""
        output = ""
        while True:
            try:
                item = self.io_q.get(True, 1)
            except Empty:
                if self.proc.poll() is not None:
                    break
            else:
                output = output + item
                match = re.search(pattern, item)
                if match is not None:
                    return match.group(), output
        return None, output

    def sendline(self, s='')
        raise NotImplementedError

