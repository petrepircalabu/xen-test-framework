#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Basic DomU test
Runs a domain and checks the output for a spcific pattern.
"""

import os
import StringIO

from xtf import all_environments
from xtf.exceptions import RunnerError
from xtf.logger import Logger
from xtf.test import TestInstance, TestInfo, TestResult
from xtf.xl_domu import XLDomU

class DomuTestInstance(TestInstance):
    """ Object representing a single DOMU test. """

    def __init__(self, env, name, variation):
        super(DomuTestInstance, self).__init__(name)

        self.env, self.variation = env, variation

        if self.env is None:
            raise RunnerError("No environment for '%s'" % (self.name, ))

        self.domu = XLDomU(self.cfg_path())
        self.results_mode = 'console'
        self.logpath = None
        if not Logger().quiet:
            self.output = StringIO.StringIO()
        else:
            self.output = None

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

    def set_up(self, opts, result):
        self.results_mode = opts.results_mode
        if self.results_mode not in ['console', 'logfile']:
            raise RunnerError("Unknown mode '%s'" % (opts.results_mode, ))

        self.logpath = os.path.join(opts.logfile_dir,
                          opts.logfile_pattern.replace("%s", str(self)))
        self.domu.create()

    def run(self, result):
        """Executes the test instance"""
        run_test = { "console": self._run_test_console,
                     "logfile": self._run_test_logfile,
        }.get(self.results_mode, None)

        run_test(result)

    def clean_up(self, result):
        if self.output:
            self.output.close()

        # wait for completion
        if not self.domu.cleanup():
            result.set(TestResult.CRASH)

    def _run_test_console(self, result):
        """ Run a specific, obtaining results via xenconsole """

        console = self.domu.console(self.output)

        # start the domain
        self.domu.unpause()
        value = console.expect(self.result_pattern())

        if self.output is not None:
            Logger().log(self.output.getvalue())

        result.set(value)

    def _run_test_logfile(self, result):
        """ Run a specific test, obtaining results from a logfile """

        Logger().log("Using logfile '%s'" % (self.logpath, ))

        fd = os.open(self.logpath, os.O_CREAT | os.O_RDONLY, 0644)
        logfile = os.fdopen(fd)
        logfile.seek(0, os.SEEK_END)

        self.domu.unpause()

        # wait for completion
        if not self.domu.cleanup():
            result.set(TestResult.CRASH)

        line = ""
        for line in logfile.readlines():
            line = line.rstrip()
            Logger().log(line)

            if "Test result:" in line:
                print ""
                break

        logfile.close()

        result.set(TestInstance.parse_result(line))


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
            if env not in all_environments:
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
                    res.append(self.instance_class(env, self.name, vary))
        else:
            res = [ self.instance_class(env, self.name, None)
                    for env in envs ]
        return res

    def __repr__(self):
        return "%s(%s)" % (self.__class__.__name__, self.name, )
