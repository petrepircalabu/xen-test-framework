#!/usr/bin/env python
# -*- coding: utf-8 -*-

class Singleton(type):
    """Singleton meta class"""
    _instances = {}
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]

class Logger(object):
    """Logger class for XTF."""
    __metaclass__ = Singleton

    def initialize(self, opts):
        """Initialize logger"""
        self.quiet = opts.quiet

    def log(self, message):
        """Display the message"""
        if not self.quiet:
            print message
