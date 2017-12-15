#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Executable test classes

Spawns a process and waits for a specific pattern
"""

import pexpect

from   xtf.test import TestInstance, TestInfo, TestResult

class ExecutableTestInstance(TestInstance):
    """Executable Test Instance"""
    def __init__(self, cmd, args, pattern, logfile):
        super(ExecutableTestInstance, self).__init__()
        
        self._cmd = cmd
        self._args = args
        self._pattern = pattern
        self._proc = None
        self._logfile = logfile

    def _handle_proc(self, result)
        value = self._proc.expect(self._pattern + [pexpect.TIMEOUT, pexpect.EOF])

        if value >= len(self.pattern):
            result.set(TestResult.CRASH)
            return

        result.set(TestResult.SUCCESS)

    def run(self, opts, result):
        """Executes the test instance""" 
        self._proc = pexpect.spawn(self._cmd, self._args, logfile = logfile)
        
        if self._proc is None:
            result.set(TestResult.ERROR)
            return

        self._handle_proc(result)
