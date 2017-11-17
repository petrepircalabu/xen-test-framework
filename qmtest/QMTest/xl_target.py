########################################################################
# Imports
#######################################################################

import imp
import os

from   qm.test.target import Target
from   subprocess import Popen, PIPE, call as subproc_call
from   qm.temporary_directory import TemporaryDirectory

########################################################################
# Classes
#######################################################################

class XLTarget(Target):
    def __init__(self, database, properties):

        super(XLTarget, self).__init__(database, properties)
        self.__temporary_directory = TemporaryDirectory()


    def IsIdle(self):
        """Return true if the target is idle.

        returns -- True if the target is idle.  If the target is idle,
        additional tasks may be assigned to it."""

        # The target is always idle when this method is called since
        # whenever it asked to perform a task it blocks the caller.
        return 1


    def _GetTemporaryDirectory(self):

        return self.__temporary_directory.GetPath()
