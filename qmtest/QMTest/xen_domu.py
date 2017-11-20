"""XEN DomU Abstract class"""
########################################################################
# Imports
########################################################################

########################################################################
# Classes
########################################################################

class XENDomU(object):
    """"XEN DomU abstract class"""

    def __init__(self):
        pass

    def create(self):
        """Creates the domain"""
        raise NotImplementedError

    def destroy(self):
        """Destroys the domain"""
        raise NotImplementedError

    def unpause(self):
        """Unpauses the domain"""
        raise NotImplementedError
