#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, os.path as path

try:
    import json
except ImportError:
    import simplejson as json

from xtf.exceptions import RunnerError
from xtf.domu_test import DomuTestInfo

# Cached test json from disk
_all_test_info = {}

def get_all_test_info():
    """ Returns all available test info instances """

    if not _all_test_info:
        raise RunnerError("No available test info")

    return _all_test_info

def gather_all_test_info():
    """ Open and collate each info.json """

    for test in os.listdir("tests"):

        info_file = None
        try:

            # Ignore directories which don't have a info.json inside them
            try:
                info_file = open(path.join("tests", test, "info.json"))
            except IOError:
                continue

            # Ignore tests which have bad JSON
            try:
                test_info = DomuTestInfo(json.load(info_file))

                if test_info.name != test:
                    continue

            except (ValueError, KeyError, TypeError):
                continue

            _all_test_info[test] = test_info

        finally:
            if info_file:
                info_file.close()

