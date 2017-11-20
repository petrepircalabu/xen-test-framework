"""XTF Simple test."""
########################################################################
# Imports
########################################################################

from   qm.test.test import Test

########################################################################
# Classes
########################################################################

class XTFSimpleTest(Test):
    """XTF Simple Test."""

    def Run(self, context, result):
        """Run the test."""
        domu = context["domu_object"]
        domu.unpause()
