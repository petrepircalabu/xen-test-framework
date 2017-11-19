""" XL DOMU Resource."""
########################################################################
# Imports
#######################################################################

import os.path

from   qm.fields import TextField
from   qm.test.resource import Resource

########################################################################
# Classes
#######################################################################

class XLDomUResource(Resource):
    """A DomU resource loads an image."""

    arguments = [
        TextField(
            name="conf_file",
            title="XL configuration file",
        ),
    ]

    def SetUp(self, context, result):

        xtf_root = context.get("XTF_ROOT")
        if xtf_root is None:
            context["XTF_ROOT"] = os.getcwd()

        conf = os.path.normpath(os.path.join(context["XTF_ROOT"], self.conf_file))
        if not os.path.isfile(conf):
            result.Fail("Invalid XL configuration file.", {"file": conf})
            return

    def CleanUp(self, result):
        pass
