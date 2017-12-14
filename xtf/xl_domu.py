#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""XL DomU class"""
########################################################################
# Imports
########################################################################

import imp
import os.path
import time

from   subprocess import Popen, PIPE

import pexpect

from   xtf.exceptions import RunnerError
from   xtf.logger import Logger

########################################################################
# Functions
########################################################################

def _run_cmd(args):
    """Execute command using Popen"""
    proc = Popen(args, stdout = PIPE, stderr = PIPE)
    Logger().log("Executing '%s'" % (" ".join(args), ))
    _, stderr = proc.communicate()
    return proc.returncode, _, stderr

def _xl_create(xl_conf_file, paused, fg):
    """Creates a XEN Domain using the XL toolstack"""
    args = ['xl', 'create']
    if paused:
        args.append('-p')
    if fg:
        args.append('-F')
    args.append(xl_conf_file)
    ret, _, stderr = _run_cmd(args)
    if ret:
        raise RunnerError("_xl_create", ret, _, stderr)

def _xl_dom_id(xl_dom_name):
    """Returns the ID of a XEN domain specified by name"""
    args = ['xl', 'domid', xl_dom_name]
    ret, _, stderr = _run_cmd(args)
    if ret:
        raise RunnerError("_xl_dom_id", ret, _, stderr)
    return long(_)

def _xl_destroy(domid):
    """Destroy the domain specified by domid"""
    args = ['xl', 'destroy', str(domid)]
    ret, _, stderr = _run_cmd(args)
    if ret:
        raise RunnerError("_xl_destroy", ret, _, stderr)

def _xl_unpause(domid):
    """Unpauses the domain specified by domid"""
    args = ['xl', 'unpause', str(domid)]
    ret, _, stderr = _run_cmd(args)
    if ret:
        raise RunnerError("_xl_unpause", ret, _, stderr)

def _is_alive(domid):
    """Checks if the domain is alive using xenstore."""
    args = ['xenstore-exists', os.path.join("/local/domain/", str(domid))]
    ret = _run_cmd(args)[0]
    return ret != 0


########################################################################
# Classes
########################################################################

class XLDomU(object):
    """XEN DomU implementation using the XL toolstack"""

    def __init__(self, conf):
        super(XLDomU, self).__init__()
        self.__xl_conf_file = conf
        self.dom_id = 0
        code = open(conf)
        self.__config = imp.new_module(conf)
        exec code in self.__config.__dict__
        self.__console = None

    def create(self, paused=True, fg=False):
        """Creates the XEN domain."""
        _xl_create(self.__xl_conf_file, paused, fg)
        self.dom_id = _xl_dom_id(self.__config.name)

    def cleanup(self, timeout=10):
        """Destroys the domain."""

        if self.dom_id == 0:
            return

        for _ in xrange(timeout):
            if not _is_alive(self.dom_id):
                return
            time.sleep(1)

        if _is_alive(self.dom_id):
            _xl_destroy(self.dom_id)
            self.dom_id = 0

    def unpause(self):
        """Unpauses the domain."""
        _xl_unpause(self.dom_id)

    def console(self, logfile=None):
        """Creates the domain_console handler."""
        if self.__console is None:
            self.__console = pexpect.spawn('xl', ['console', str(self.dom_id)],
                    logfile=logfile)
        return self.__console
