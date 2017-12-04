#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" mkinfo.py

    Generates a test info json file.
    The script is ran at build stage using the parameters specified
    in the test's Makefile.
"""

import json
import sys
import shlex
from   optparse import OptionParser

def main():
    """ Main entrypoint """
    # Avoid wrapping the epilog text
    OptionParser.format_epilog = lambda self, formatter: self.epilog

    parser = OptionParser(
        usage = "%prog [OPTIONS] out_file",
        description = "Xen Test Framework json generation tool",
        )

    parser.add_option("-n", "--name", action = "store",
                      dest = "name",
                      help = "Test name",
                      )
    parser.add_option("-c", "--class", action = "store",
                      dest = "class_name",
                      help = "Test class name",
                      )
    parser.add_option("-t", "--category", action = "store",
                      dest = "cat",
                      help = "Test category",
                      )
    parser.add_option("-e", "--environments", action = "store",
                      dest = "envs",
                      help = "Test environments (e.g hvm64, pv64 ...)",
                      )
    parser.add_option("-v", "--variations", action = "store",
                      dest = "variations",
                      help = "Test variations",
                      )
    parser.add_option("-x", "--extra", action = "store",
                      dest = "extra",
                      help = "Test specific parameters",
                      )

    opts, args = parser.parse_args()
    template = {
        "name": opts.name,
        "class_name": opts.class_name,
        "category": opts.cat,
        "environments": [],
        "variations": [],
        "extra": {}
        }

    if opts.envs:
        template["environments"] = opts.envs.split(" ")
    if opts.variations:
        template["variations"] = opts.variations.split(" ")
    if opts.extra:
        template["extra"] = dict([(e.split('=',1))
                                 for e in shlex.split(opts.extra)])

    open(args[0], "w").write(
        json.dumps(template, indent=4, separators=(',', ': '))
        + "\n"
        )

if __name__ == "__main__":
    sys.exit(main())
