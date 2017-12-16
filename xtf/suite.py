#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os, os.path as path
import sys
import imp

try:
    import json
except ImportError:
    import simplejson as json

from xtf.exceptions import RunnerError

# Cached test json from disk
_all_test_info = {}

def _load_module(name):
    """Loads module dynamically"""
    components = name.split(".")
    module_path = sys.path

    for index in xrange(len(components)):
        module_name = components[index]
        module = sys.modules.get(module_name)
        if module:
            if hasattr(module, '__path__'):
                module_path = module.__path__
            continue

        try:
            mod_file, filename, description = imp.find_module(module_name,
                                                              module_path)
            module = imp.load_module(module_name, mod_file, filename,
                                     description)
            if hasattr(module, '__path__'):
                module_path = module.__path__
        finally:
            if mod_file:
                mod_file.close()

    return module

def _load_class(name):
    """Loads python class dynamically"""
    components = name.split(".")
    class_name = components[-1]
    module = _load_module(".".join(components[:-1]))

    try:
        cls = module.__dict__[class_name]
        return cls
    except KeyError:
        return None


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
                try:
                    info_file = open(path.join("tests", test, "host.json"))
                except IOError:
                    continue

            # Ignore tests which have bad JSON
            try:
                json_info = json.load(info_file)
                test_class = _load_class(json_info["class_name"])
                test_info = test_class(json_info)

                if test_info.name != test:
                    continue

            except (ValueError, KeyError, TypeError):
                continue

            _all_test_info[test] = test_info

        finally:
            if info_file:
                info_file.close()

