#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Executable test classes

Spawns a process and waits for a specific pattern
"""

import StringIO
import pexpect

from xtf.logger import Logger
from xtf.test import TestInstance, TestInfo, TestResult

class ExecutableTestInstance(TestInstance):
    """Executable Test Instance"""
    def __init__(self, name, cmd, args, pattern):
        super(ExecutableTestInstance, self).__init__(name)

        self._cmd = cmd
        self._args = [x.encode('utf-8') for x in args]
        self._pattern = [x.encode('utf-8') for x in pattern]
        self._proc = None
        self.env = "dom0"
        self.output = StringIO.StringIO()

    def __repr__(self):
        return "test-%s-%s" %(self.env, self.name)

    def wait_pattern(self, pattern, value):
        """Expect the pattern given as parameter."""
        value = self._proc.expect(pattern + [pexpect.TIMEOUT, pexpect.EOF])

    def set_up(self, opts, result):
        self._proc = pexpect.spawn(self._cmd, self._args, logfile = self.output)
        print self._cmd, self._args

        if self._proc is None:
            result.set(TestResult.ERROR)

    def run(self, result):
        """Executes the test instance"""
        value = 0
        self.wait_pattern(self._pattern, value)

        if value >= len(pattern):
            result.set(TestResult.FAILURE)
            return
        result.set(TestResult.SUCCESS)

    def clean_up(self, result):
        if self.output:
            Logger().log(self.output.getvalue())
            self.output.close()

class ExecutableTestInfo(TestInfo):
    """ Object representing a tests info.json, in a more convenient form. """

    def __init__(self, test_json):
        super(ExecutableTestInfo, self).__init__(test_json)
        self.instance_class = ExecutableTestInstance

        cmd = test_json["cmd"]
        if not isinstance(cmd, (str, unicode)):
            raise TypeError("Expected string for 'cmd', got '%s')"
                            % (type(cmd), ))
        self.cmd = cmd

        args = test_json["args"]
        if not isinstance(args, list):
            raise TypeError("Expected list for 'args', got '%s')"
                            % (type(args), ))
        self.args = args

        pattern = test_json["pattern"]
        if not isinstance(pattern, list):
            raise TypeError("Expected list for 'pattern', got '%s')"
                            % (type(pattern), ))
        self.pattern = pattern

    def all_instances(self, env_filter = None, vary_filter = None):
        """Returns an ExecutableTestInstance object"""
        return [self.instance_class(self.name, self.cmd, self.args,
                                    self.pattern),]
