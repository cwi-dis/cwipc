import os
from setuptools import setup

def get_version():
    CWIPC_VERSION="8.0+unknown"
    if 'CWIPC_VERSION' in os.environ:
        CWIPC_VERSION=os.environ['CWIPC_VERSION']
    return CWIPC_VERSION

setup(
    version=get_version(),
)
