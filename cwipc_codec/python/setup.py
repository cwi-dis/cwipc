import os
from setuptools import setup

def get_version():
    CWIPC_VERSION="7.7+unknown"
    if 'CWIPC_VERSION' in os.environ:
        CWIPC_VERSION=os.environ['CWIPC_VERSION']
    return CWIPC_VERSION

setup(
    version=get_version(),
)
