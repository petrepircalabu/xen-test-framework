#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, os.path as path
from subprocess import Popen, PIPE, call as subproc_call

from xtf import all_environments
from xtf.exceptions import RunnerError
from xtf.test import TestInstance, TestInfo

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

        cmd = ['xl', 'create', '-p', self.cfg_path()]
        if not opts.quiet:
            print "Executing '%s'" % (" ".join(cmd), )

        create = Popen(cmd, stdout = PIPE, stderr = PIPE)
        _, stderr = create.communicate()

        if create.returncode:
            if opts.quiet:
                print "Executing '%s'" % (" ".join(cmd), )
            print stderr
            raise RunnerError("Failed to create VM")

        cmd = ['xl', 'console', self.vm_name()]
        if not opts.quiet:
            print "Executing '%s'" % (" ".join(cmd), )

        console = Popen(cmd, stdout = PIPE)

        cmd = ['xl', 'unpause', self.vm_name()]
        if not opts.quiet:
            print "Executing '%s'" % (" ".join(cmd), )

        rc = subproc_call(cmd)
        if rc:
            if opts.quiet:
                print "Executing '%s'" % (" ".join(cmd), )
            raise RunnerError("Failed to unpause VM")

        stdout, _ = console.communicate()

        if console.returncode:
            raise RunnerError("Failed to obtain VM console")

        lines = stdout.splitlines()

        if lines:
            if not opts.quiet:
                print "\n".join(lines)
                print ""

        else:
            return "CRASH"

        return TestInstance.interpret_result(lines[-1])

    def _run_test_logfile(self, opts):
        """ Run a specific test, obtaining results from a logfile """

        logpath = path.join(opts.logfile_dir,
                            opts.logfile_pattern.replace("%s", str(self)))

        if not opts.quiet:
            print "Using logfile '%s'" % (logpath, )

        fd = os.open(logpath, os.O_CREAT | os.O_RDONLY, 0644)
        logfile = os.fdopen(fd)
        logfile.seek(0, os.SEEK_END)

        cmd = ['xl', 'create', '-F', self.cfg_path()]
        if not opts.quiet:
            print "Executing '%s'" % (" ".join(cmd), )

        guest = Popen(cmd, stdout = PIPE, stderr = PIPE)

        _, stderr = guest.communicate()

        if guest.returncode:
            if opts.quiet:
                print "Executing '%s'" % (" ".join(cmd), )
            print stderr
            raise RunnerError("Failed to run test")

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
