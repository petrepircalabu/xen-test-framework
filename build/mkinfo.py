#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, os, json

# Usage: mkcfg.py $OUT $NAME $CLASS $CATEGORY $ENVS $VARIATIONS
_, out, name, class_name, cat, envs, variations = sys.argv

template = {
    "name": name,
    "class_name": class_name,
    "category": cat,
    "environments": [],
    "variations": [],
    }

if envs:
    template["environments"] = envs.split(" ")
if variations:
    template["variations"] = variations.split(" ")

open(out, "w").write(
    json.dumps(template, indent=4, separators=(',', ': '))
    + "\n"
    )
