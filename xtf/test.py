#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re
from xtf import all_categories

class TestInstance(object):
    """Base class for a XTF Test Instance object"""
    # All results of a test, keep in sync with C code report.h.
    # Notes:
    #  - WARNING is not a result on its own.
    #  - CRASH isn't known to the C code, but covers all cases where a valid
    #    result was not found.
    all_results = ['SUCCESS', 'SKIP', 'ERROR', 'FAILURE', 'CRASH']

    @staticmethod
    def interpret_result(logline):
        """ Interpret the final log line of a guest for a result """

        if "Test result:" not in logline:
            return "CRASH"

        for res in TestInstance.all_results:
            if res in logline:
                return res

        return "CRASH"

    @staticmethod
    def result_pattern():
        """Compiles the test result regex pattern."""
        return re.compile('(?<=Test result: )({})'.format(
            '|'.join(TestInstance.all_results)))

    def __init__(self):
        pass

    def run(self, opts):
        """Runs the Test Instance."""
        raise NotImplementedError

class TestInfo(object):
    """Base class for a XTF Test Info object.
    It represents a tests info.json, in a more convenient form.
    """

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

    def all_instances(self, env_filter = None, vary_filter = None):
        """Return a list of TestInstances, for each supported environment.
        Optionally filtered by env_filter.  May return an empty list if
        the filter doesn't match any supported environment.
        """
        raise NotImplementedError
