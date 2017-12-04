#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os.path as path

from xtf import all_categories
from xtf import all_environments
from xtf.exceptions import RunnerError

class TestInstance(object):
    """ Object representing a single test. """

    def __init__(self, env, name, variation, test_info):
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


class TestInfo(object):
    """ Object representing a tests info.json, in a more convenient form. """

    def __init__(self, test_json):
        """Parse and verify 'test_json'.

        May raise KeyError, TypeError or ValueError.
        """

        name = test_json["name"]
        if not isinstance(name, basestring):
            raise TypeError("Expected string for 'name', got '%s'"
                            % (type(name), ))
        self.name = name

        cat = test_json["category"]
        if not isinstance(cat, basestring):
            raise TypeError("Expected string for 'category', got '%s'"
                            % (type(cat), ))
        if not cat in all_categories:
            raise ValueError("Unknown category '%s'" % (cat, ))
        self.cat = cat

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
                    res.append(TestInstance(env, self.name, vary, self))
        else:
            res = [ TestInstance(env, self.name, None, self)
                    for env in envs ]
        return res

    def __repr__(self):
        return "TestInfo(%s)" % (self.name, )
