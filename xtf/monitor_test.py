#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" Monitor test classes.

    The monitor test spawns an test monitor (event channel handler application)
    instance and runs a DomU image which interacts with it.
"""

import os.path as path
from   subprocess import Popen

import StringIO

from   xtf.exceptions import RunnerError
from   xtf.domu_test import DomuTestInstance, DomuTestInfo
from   xtf.logger import Logger
from   xtf.test import TestResult
from   xtf.xl_domu import XLDomU

class MonitorTestInstance(DomuTestInstance):
    """Monitor test instance"""

    def __init__(self, env, name, variation, test_info):
        super(MonitorTestInstance, self).__init__(env, name, variation,
                                                  test_info)

        self.monitor_args = (test_info.extra["monitor_args"]
                             .replace("@@VM_PATH@@", self.vm_path())
        )

    def vm_path(self):
        """ Return the VM path. """
        return path.join("tests", self.name, repr(self))

    def monitor_path(self):
        """ Return the path to the test's monitor app if applicable. """
        return path.join("tests", self.name, "test-monitor-" + self.name)

    def start_monitor(self, dom_id):
        """ Starts the monitor application. """
        cmd = [" ".join([self.monitor_path(), self.monitor_args, str(dom_id)])]
        Logger().log("Executing '%s'" % (" ".join(cmd), ))
        return Popen(cmd, shell=True)

    def run(self, opts):
        # set up the domain
        domu = XLDomU(self.cfg_path())
        domu.create()

        if not Logger().quiet:
            output = StringIO.StringIO()
        else:
            output = None

        console = domu.console(output)

        # start the monitor & domain
        monitor = self.start_monitor(domu.dom_id)

        # domu.unpause()
        result = console.expect(self.result_pattern())

        if monitor.returncode:
            raise RunnerError("Failed to start monitor application")

        if output is not None:
            Logger().log(output.getvalue())
            output.close()

        return TestResult(result)

class MonitorTestInfo(DomuTestInfo):
    """Monitor test info"""

    def __init__(self, test_json):
        super(MonitorTestInfo, self).__init__(test_json)
        self.instance_class = MonitorTestInstance
