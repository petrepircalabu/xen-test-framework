#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, os.path as path
import time

from xtf import all_environments
from xtf.exceptions import RunnerError
from xtf.logger import Logger
from xtf.test import TestInstance, TestInfo
from xtf.xl_domu import XLDomU

class DomuTestInstance(TestInstance):
    """ Object representing a single DOMU test. """

    def __init__(self, env, name, variation, test_info):
        super(DomuTestInstance, self).__init__()

        self.env, self.name, self.variation = env, name, variation

        if self.env is None:
            raise RunnerError("No environment for '%s'" % (self.name, ))

        if self.variation is None and test_info.variations:
            raise RunnerError("Test '%s' has variations, but none specified"
                              % (self.name, ))

    def vm_name(self):
        """ Return the VM name as `xl` expects it. """
        return repr(self)

    def cfg_path(self):
        """ Return the path to the `xl` config file for this test. """
        return path.join("tests", self.name, repr(self) + ".cfg")

    def __repr__(self):
        if not self.variation:
            return "test-%s-%s" % (self.env, self.name)
        else:
            return "test-%s-%s~%s" % (self.env, self.name, self.variation)

    def __hash__(self):
        return hash(repr(self))

    def __cmp__(self, other):
        return cmp(repr(self), repr(other))

    def _notify_domain_create(self):
        """Called after the domain was created and before it is unpaused.
           Returns false if some external dependencies failed.
        """
        return True

    def run(self, opts):
        """Executes the test instance"""
        run_test = { "console": self._run_test_console,
                     "logfile": self._run_test_logfile,
        }.get(opts.results_mode, None)

        if run_test is None:
            raise RunnerError("Unknown mode '%s'" % (opts.mode, ))

        return run_test(opts)

    def _run_test_console(self, opts):
        """ Run a specific, obtaining results via xenconsole """

        # set up the domain
        domu = XLDomU(self.cfg_path())
        domu.create()
        console = domu.console()

        if not self._notify_domain_create():
            return "ERROR"

        # start the domain
        domu.unpause()
        value, _ = console.wait(self.result_pattern())
        if value is None:
            value = "CRASH"

        Logger().log(_)

        return value

    def _run_test_logfile(self, opts):
        """ Run a specific test, obtaining results from a logfile """

        logpath = path.join(opts.logfile_dir,
                            opts.logfile_pattern.replace("%s", str(self)))

        if not opts.quiet:
            print "Using logfile '%s'" % (logpath, )

        fd = os.open(logpath, os.O_CREAT | os.O_RDONLY, 0644)
        logfile = os.fdopen(fd)
        logfile.seek(0, os.SEEK_END)

        domu = XLDomU(self.cfg_path())
        domu.create()

        if not self._notify_domain_create():
            return "ERROR"

        domu.unpause()
        # wait for completion
        for _ in xrange(5):
            try:
                domu.domname()
            except RunnerError:
                break
            else:
                time.sleep(1)

        line = ""
        for line in logfile.readlines():

            line = line.rstrip()
            if not opts.quiet:
                print line

            if "Test result:" in line:
                print ""
                break

        logfile.close()

        return TestInstance.interpret_result(line)


class DomuTestInfo(TestInfo):
    """ Object representing a tests info.json, in a more convenient form. """

    def __init__(self, test_json):
        """Parse and verify 'test_json'.

        May raise KeyError, TypeError or ValueError.
        """

        super(DomuTestInfo, self).__init__(test_json)
        self.instance_class = DomuTestInstance

        envs = test_json["environments"]
        if not isinstance(envs, list):
            raise TypeError("Expected list for 'environments', got '%s'"
                            % (type(envs), ))
        if not envs:
            raise ValueError("Expected at least one environment")
        for env in envs:
            if not env in all_environments:
                raise ValueError("Unknown environments '%s'" % (env, ))
        self.envs = envs

        variations = test_json["variations"]
        if not isinstance(variations, list):
            raise TypeError("Expected list for 'variations', got '%s'"
                            % (type(variations), ))
        self.variations = variations

        extra = test_json["extra"]
        if not isinstance(extra, dict):
            raise TypeError("Expected dict for 'extra', got '%s'"
                            % (type(extra), ))
        self.extra = extra

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
                    res.append(self.instance_class(env, self.name, vary, self))
        else:
            res = [ self.instance_class(env, self.name, None, self)
                    for env in envs ]
        return res

    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__, self.name, )
