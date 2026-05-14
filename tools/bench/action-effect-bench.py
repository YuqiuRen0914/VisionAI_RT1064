#!/usr/bin/env python3
"""CLI wrapper for action_effect_bench."""

from __future__ import annotations

import sys

import action_effect_bench


if __name__ == "__main__":
    raise SystemExit(action_effect_bench.main(sys.argv[1:]))
