"""XTF Simple test."""
########################################################################
# Imports
########################################################################
import os.path
import re
import time

from   qm.fields import TextField, SetField
from   qm.test.test import Test
from   qm.test.result import Result
from   xl_domu import XLDomU
from   xtf_utils import XTFError

########################################################################
# Classes
########################################################################

class XTFSimpleTest(Test):
    """XTF Simple Test."""

    arguments = [
        TextField(
            name="conf_file",
            title="XL configuration file",
            ),
        SetField(TextField(
            name="tags",
            title="test tags",
            )),
    ]

    all_results = ['SUCCESS', 'SKIP', 'ERROR', 'FAILURE', 'CRASH']

    def Run(self, context, result):
        """Run the test."""

        xtf_root = context.get("XTF_ROOT")
        if xtf_root is None:
            context["XTF_ROOT"] = os.getcwd()

        conf = os.path.normpath(os.path.join(context["XTF_ROOT"],
            self.conf_file))
        if not os.path.isfile(conf):
            result.Fail("Invalid XL configuration file.", {"file": conf})
            return

        try:
            # Set up the domain
            domu = XLDomU(conf)
            domu.create()
            console = domu.console()

            # Start the domain
            domu.unpause()
            pattern = re.compile('(?<=Test result: )({})'.format('|'.join(XTFSimpleTest.all_results)))
            value, output = console.wait(pattern)
            if value is None:
                value = "CRASH"

        except XTFError as e:
            e.Annotate(result)
            result.Fail("Error while executing test")

        result.Annotate({"output": output})

        if value == 'SUCCESS':
            result.SetOutcome(Result.PASS)
        elif value == 'SKIP':
            result.SetOutcome(Result.UNTESTED)
        elif value == 'FAILURE':
            result.SetOutcome(Result.FAIL)
        elif value == 'ERROR' or value == 'CRASH':
            result.SetOutcome(Result.ERROR)

        # Cleanup
        for _ in xrange(5):
            try:
                domu.domname()
            except XTFError as er:
                return
            else:
                time.sleep(1)

        try:
            domu.destroy()
        except XTFError as er:
            # Ignore error
            pass
