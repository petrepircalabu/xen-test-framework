#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" XTF utils """

import threading

class XTFAsyncCall(threading.Thread):
    def __init__(self, group=None, target=None, name=None, args=(), kwargs={}):
        super(XTFAsyncCall, self).__init__(group, target, name, args, kwargs)
        self._return = None
    def run(self):
        if self._Thread__target is not None:
            self._return = self._Thread__target(*self._Thread__args,
                                                **self._Thread__kwargs)
    def join(self):
        threading.Thread.join(self)
        return self._return
