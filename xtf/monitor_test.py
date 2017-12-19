#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" Monitor test classes.

    The monitor test spawns an test monitor (event channel handler application)
    instance and runs a DomU image which interacts with it.
"""

import os
import threading
from   subprocess import Popen

from   xtf.exceptions import RunnerError
from   xtf.domu_test import DomuTestInstance, DomuTestInfo
from   xtf.executable_test import ExecutableTestInstance
from   xtf.logger import Logger
from   xtf.test import TestResult, TestInstance

class MonitorTestInstance(TestInstance):
    """Monitor test instance"""

    def __init__(self, env, name, variation, monitor_args):
        super(MonitorTestInstance, self).__init__(name)

        self.env, self.variation = env, variation

        if self.env is None:
            raise RunnerError("No environment for '%s'" % (self.name, ))

        self.monitor_args = monitor_args.replace("@@VM_PATH@@", self.vm_path())

        self.domu_test = None
        self.monitor_test = None

    def vm_name(self):
        """ Return the VM name as `xl` expects it. """
        return repr(self)

    def cfg_path(self):
        """ Return the path to the `xl` config file for this test. """
        return os.path.join("tests", self.name, repr(self) + ".cfg")

    def __repr__(self):
        if self.variation:
            return "test-%s-%s~%s" % (self.env, self.name, self.variation)
        return "test-%s-%s" % (self.env, self.name)

    def vm_path(self):
        """ Return the VM path. """
        return os.path.join("tests", self.name, repr(self))

    def monitor_path(self):
        """ Return the path to the test's monitor app if applicable. """
        return os.path.join("tests", self.name, "test-monitor-" + self.name)

    def start_monitor(self, dom_id):
        """ Starts the monitor application. """
        cmd = [" ".join([self.monitor_path(), self.monitor_args, str(dom_id)])]
        Logger().log("Executing '%s'" % (" ".join(cmd), ))
        return Popen(cmd, shell=True)

    def set_up(self, opts, result):
        self.domu_test = DomuTestInstance(self.env, self.name, self.variation)
        self.domu_test.set_up(opts, result)
        if result != TestResult.SUCCESS:
            return

        monitor_cmd = ' '.join([self.monitor_path(), self.monitor_args,
                                str(self.domu_test.domu.dom_id)])

        self.monitor_test = ExecutableTestInstance(self.name, '/bin/sh',
                                                   ['-c', monitor_cmd], "")
        self.monitor_test.set_up(opts, result)
        self.monitor_test.wait_pattern(['Monitor initialization complete.'],
                                       result)

    def run(self, result):
        r1 = TestResult()
        r2 = TestResult()
        value = 0

        t1 = threading.Thread(target=self.domu_test.run, args=(r1,))
        t2 = threading.Thread(target=self.monitor_test.wait_pattern,
                args=(self.result_pattern(), value))

        for th in (t1, t2):
            th.start()

        for th in (t1, t2):
            th.join()

        r2.set(value)

        print r1
        print r2

    def clean_up(self, result):
        if self.domu_test:
            self.domu_test.clean_up(result)

        if self.monitor_test:
            self.monitor_test.clean_up(result)

class MonitorTestInfo(DomuTestInfo):
    """Monitor test info"""

    def __init__(self, test_json):
        super(MonitorTestInfo, self).__init__(test_json)
        self.instance_class = MonitorTestInstance
        self.monitor_args = self.extra['monitor_args']

    def all_instances(self, env_filter = None, vary_filter = None):
        """Return a list of TestInstances, for each supported environment.
        Optionally filtered by env_filter.  May return an empty list if
        the filter doesn't match any supported environment.
        """

        if env_filter:
            envs = set(env_filter).intersection(self.envs)
        else:
            envs = self.envs

        if vary_filter:
            variations = set(vary_filter).intersection(self.variations)
        else:
            variations = self.variations

        res = []
        if variations:
            for env in envs:
                for vary in variations:
                    res.append(self.instance_class(env, self.name, vary,
                                                   self.monitor_args))
        else:
            res = [ self.instance_class(env, self.name, None, self.monitor_args)
                    for env in envs ]
        return res
