########################################################################
# Imports
########################################################################

import qm.common
import os.path
import string
import sys

from   qm.test.test import Test
from   qm.test.result import Result
from   qm.fields import TextField, SetField
from   xtf_utils import XTFError, XTFResult

########################################################################
# Classes
########################################################################

class XTFSimpleTest(Test):

    def Run(self, context, result):
        """Run the test."""

        result.Annotate({"gogu": context["gogu"]})
