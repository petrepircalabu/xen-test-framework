""" XL DOMU Resource."""
########################################################################
# Imports
#######################################################################

import os.path

from   qm.fields import TextField
from   qm.test.resource import Resource
from   xl_domu import XLDomU
from   xtf_utils import XTFError

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

    def __init__(self, arguments = None, **args):
        super(XLDomUResource, self).__init__(arguments, **args)
        self.__domu = None

    def SetUp(self, context, result):

        xtf_root = context.get("XTF_ROOT")
        if xtf_root is None:
            context["XTF_ROOT"] = os.getcwd()

        conf = os.path.normpath(os.path.join(context["XTF_ROOT"],
            self.conf_file))
        if not os.path.isfile(conf):
            result.Fail("Invalid XL configuration file.", {"file": conf})
            return

        try:
            self.__domu = XLDomU(conf)
            self.__domu.create()
        except XTFError as e:
            e.Annotate(result)
            result.Fail("Error while executing test")

        context["domu_object"] = self.__domu

    def CleanUp(self, result):
        if self.__domu != None:
            try:
                self.__domu.destroy()
            except XTFError as e:
                e.Annotate(result)
                result.Fail("Error while executing test")
