#!/usr/bin/env python
# -*- coding: utf-8 -*-

# All test categories
default_categories     = set(("functional", "xsa"))
non_default_categories = set(("special", "utility", "in-development"))
all_categories         = default_categories | non_default_categories

# All test environments
pv_environments        = set(("pv64", "pv32pae"))
hvm_environments       = set(("hvm64", "hvm32pae", "hvm32pse", "hvm32"))
all_environments       = pv_environments | hvm_environments
