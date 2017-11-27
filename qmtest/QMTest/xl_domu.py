"""XL DomU class"""
########################################################################
# Imports
########################################################################

import imp

from   Queue import Queue, Empty
from   subprocess import Popen, PIPE
from   threading import Thread
from   xen_domu import XENDomU
from   xtf_utils import XTFError

########################################################################
# Functions
########################################################################

def _run_cmd(args):
    """Execute command using Popen"""
    proc = Popen(args, stdout = PIPE, stderr = PIPE)
    _, stderr = proc.communicate()
    return proc.returncode, _, stderr

def _xl_create(xl_conf_file):
    """Creates a XEN Domain using the XL toolstack"""
    args = ['xl', 'create', '-p', xl_conf_file]
    ret, _, stderr = _run_cmd(args)
    if ret:
        raise XTFError("_xl_create", ret, _, stderr)

def _xl_dom_id(xl_dom_name):
    """Returns the ID of a XEN domain specified by name"""
    args = ['xl', 'domid', xl_dom_name]
    ret, _, stderr = _run_cmd(args)
    if ret:
        raise XTFError("_xl_dom_id", ret, _, stderr)
    return long(_)

def _xl_destroy(domid):
    """Destroy the domain specified by domid"""
    args = ['xl', 'destroy', str(domid)]
    ret, _, stderr = _run_cmd(args)
    if ret:
        raise XTFError("_xl_destroy", ret, _, stderr)

def _xl_unpause(domid):
    """Unpauses the domain specified by domid"""
    args = ['xl', 'unpause', str(domid)]
    ret, _, stderr = _run_cmd(args)
    if ret:
        raise XTFError("_xl_unpause", ret, _, stderr)

########################################################################
# Classes
########################################################################

class XLDomU(XENDomU):
    """XEN DomU implementation using the XL toolstack"""

    class XLDomuConsole(object):
        """XL Console wrapper class"""
        def __init__(self, domid):
            args = ['xl', 'console', str(domid) ]
            self.io_q = Queue()
            self.proc = Popen(args, stdout = PIPE, stderr = PIPE)
            Thread(target=self.reader_fn, name="reader",
                args=(self, self.proc.stdout,)).start()

        def reader_fn(self, stream):
            """Reader Thread"""
            line = stream.readline()
            while line:
                self.io_q.put(line)
                line = stream.readline()

            if not stream.closed:
                stream.close()

        def wait(self):
            """Print console output"""
            while True:
                try:
                    item = self.io_q.get(True, 1)
                except Empty:
                    # No output in either streams for a second. Are we done?
                    if self.proc.poll() is not None:
                        break
                else:
                    print item

    def __init__(self, conf):
        super(XLDomU, self).__init__()
        self.__xl_conf_file = conf
        self.__dom_id = 0
        code = open(conf)
        self.__config = imp.new_module(conf)
        exec code in self.__config.__dict__
        self.console = None

    def create(self):
        _xl_create(self.__xl_conf_file)
        self.__dom_id = _xl_dom_id(self.__config.name)

    def destroy(self):
        if self.__dom_id != 0:
            _xl_destroy(self.__dom_id)

    def unpause(self):
        _xl_unpause(self.__dom_id)
