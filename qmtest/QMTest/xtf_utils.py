########################################################################
# Imports
########################################################################

import qm.common

from qm.test.result import Result

########################################################################
# Classes
########################################################################

class XTFError(Exception):

    def __init__(self, context, error, msg, err_msg):

        self.__context = context
        self.__error = error
        self.__msg = msg
        self.__err_msg = err_msg

    def Annotate(self, result):

        if self.__context:
            label = self.__context + "."
        else:
            label = ""

        if self.__error:
            result.Annotate({label + "error code" : ("%d" % self.__error)})

        if self.__msg:
            result.Annotate({label + "stdout" : result.Quote(self.__msg)})

        if self.__err_msg:
            result.Annotate({label + "stdout" : result.Quote(self.__err_msg)})

class XTFResult(object):
    """XTFResult"""
    # All results of a test, keep in sync with C code report.h.
    # Notes:
    #  - WARNING is not a result on its own.
    #  - CRASH isn't known to the C code, but covers all cases where a valid
    #    result was not found.
    all_results = ['SUCCESS', 'SKIP', 'ERROR', 'FAILURE', 'CRASH']

    def __init__(self, output, result):
        self.__result = result
        self.__output = output
        self.__SetOutcome(value)

    def __SetOutcome(self, value):
        """ Set the outcome of the QMTest result """

        if value == 'SUCCESS':
            self.__result.SetOutcome(Result.PASS, cause = self.__output)
        elif value == 'SKIP':
            self.__result.SetOutcome(Result.UNTESTED, cause = self.__output)
        elif value == 'FAILURE':
            self.__result.SetOutcome(Result.FAIL, cause = self.__output)
        elif value == 'ERROR' or value == 'CRASH':
            self.__result.SetOutcome(Result.ERROR, cause = self.__output)
