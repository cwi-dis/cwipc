import sys
from typing import cast
import importlib
import importlib.util
from .abstract import cwipc_abstract_filter
from ..net.abstract import *
from . import passthrough, analyze, voxelize, transform, transform44, crop, remove_outliers, colorize, noise, simulatecams, direction, randomize_floor

all_filters = [passthrough, analyze, voxelize, transform, transform44, crop, remove_outliers, colorize, noise, simulatecams, direction, randomize_floor]

def help() -> None:
    print("A builtin filter can be specified by name (for example passthrough) or as name with arguments (for example passthrough()).", file=sys.stderr)
    print("A custom filter is specified by its Python filename (ending with .py).", file=sys.stderr)
    print("The custom filter Python source should declare a class CustomFilter() to implement the filter.", file=sys.stderr)
    print("\nThe following builtin filters are available:", file=sys.stderr)
    for filter in all_filters:
        print(filter.CustomFilter.__doc__)

def factory(filterdesc : str) -> cwipc_abstract_filter:
    """Create a filter by name (for filters without parameters).
    Or create a filter by passing the filename of a Python module implementing a CustonFilter class.
    Or create a filter by passing a string that looks like the Python code that would be used to create the filter.

    NOTE: the latter will use eval() so it is probably not very safe.
    """
    if filterdesc.lower().endswith(".py"):
        # Filter description looks like a Python source file. Import it.

        print("Loading custom filter from " + filterdesc)
        # xxxjack this name argument looks suspect
        spec = importlib.util.spec_from_file_location("module.name", filterdesc)
        assert spec
        foo = importlib.util.module_from_spec(spec)
        assert spec.loader
        spec.loader.exec_module(foo)
        return cast(cwipc_abstract_filter, foo.CustomFilter())
    if filterdesc[-1] == ')':
        openpos = filterdesc.find('(')
        filtername = filterdesc[:openpos]
        filterargs = filterdesc[openpos:]
        filterargs = eval(filterargs)
        if type(filterargs) != type(()):
            filterargs = (filterargs,)
    else:
        filtername = filterdesc
        filterargs = ()
    filter_factory = eval(filtername)
    return cast(cwipc_abstract_filter, filter_factory.CustomFilter(*filterargs))
