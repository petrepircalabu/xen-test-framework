########################################################################
# Imports
#######################################################################

from   qm.fields import *
from   qm.test.resource import *

########################################################################
# Classes
#######################################################################

class XLDomUResource(Resource):
    """A DomU resource loads an image."""

    conf_file = TextField(description="XL configuration file")

    def SetUp(self, context, result):
        context["gogu"] = "vasile"

    def CleanUp(self, result):
        pass
