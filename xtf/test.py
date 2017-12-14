#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Base XTF Test Classess
"""
import pexpect
from   xtf import all_categories

class TestResult(object):
    """
    Test result wrapper class
    All results of a test, keep in sync with C code report.h.
    Notes:
     - WARNING is not a result on its own.
     - CRASH isn't known to the C code, but covers all cases where a valid
       result was not found.
    """

    SUCCESS = 'SUCCESS'
    SKIP = 'SKIP'
    ERROR = 'ERROR'
    FAILURE = 'FAILURE'
    CRASH = 'CRASH'

    all_results = [SUCCESS, SKIP, ERROR, FAILURE, CRASH]

    def __init__(self, value=SUCCESS):
        """
        The result can be initialized using both a string value or an index
        if the index used is out-of-bounds the result will be initialized
        to CRASH
        """
        if isinstance(value, (int, long)):
            try:
                self.value = TestResult.all_results[value]
            except IndexError:
                self.value = TestResult.CRASH
        else:
            if value in TestResult.all_results:
                self.value = value
            else:
                self.value = TestResult.CRASH

    def __cmp__(self, other):
        assert isinstance(other, TestResult)
        return cmp(TestResult.all_results.index(self.value),
                   TestResult.all_results.index(other.value))

    def __repr__(self):
        return self.value

    def __hash__(self):
        return hash(repr(self))

    @staticmethod
    def success():
        """Instantiates a SUCCESS test result."""
        return TestResult(TestResult.SUCCESS)

    @staticmethod
    def skip():
        """Instantiates a SKIP test result."""
        return TestResult(TestResult.SKIP)

    @staticmethod
    def error():
        """Instantiates a ERROR test result."""
        return TestResult(TestResult.ERROR)

    @staticmethod
    def fail():
        """Instantiates a FAILURE test result."""
        return TestResult(TestResult.FAILURE)

    @staticmethod
    def crash():
        """Instantiates a CRASH test result."""
        return TestResult(TestResult.CRASH)

class TestInstance(object):
    """Base class for a XTF Test Instance object"""

    @staticmethod
    def parse_result(logline):
        """ Interpret the final log line of a guest for a result """

        if "Test result:" not in logline:
            return TestResult.crash()

        for res in TestResult.all_results:
            if res in logline:
                return TestResult(res)

        return TestResult.crash()

    @staticmethod
    def result_pattern():
        """the test result pattern."""
        return ['Test result: ' + x for x in TestResult.all_results] + [pexpect.TIMEOUT, pexpect.EOF]

    def __init__(self):
        pass

    def __hash__(self):
        return hash(repr(self))

    def __cmp__(self, other):
        return cmp(repr(self), repr(other))

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
        if cat not in all_categories:
            raise ValueError("Unknown category '%s'" % (cat, ))
        self.cat = cat

    def all_instances(self, env_filter = None, vary_filter = None):
        """Return a list of TestInstances, for each supported environment.
        Optionally filtered by env_filter.  May return an empty list if
        the filter doesn't match any supported environment.
        """
        raise NotImplementedError
