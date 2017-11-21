"""XTF Simple test."""
########################################################################
# Imports
########################################################################
import os.path

from   qm.fields import TextField
from   qm.test.test import Test
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
    ]

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
            domu = XLDomU(conf)
            domu.create()
            domu.destroy()
        except XTFError as e:
            e.Annotate(result)
            result.Fail("Error while executing test")
