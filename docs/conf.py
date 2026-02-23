# Configuration file for the Sphinx documentation builder.

import os
import subprocess
import sys

sys.path.insert(0, os.path.abspath('..'))

project = 'cwipc'
copyright = '2026, CWI'

author = 'CWI DIS Group'

extensions = [
    'myst_parser',
    'sphinx_rtd_theme',
    'breathe',
    'exhale',
]

# Support Markdown files alongside rst
source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

master_doc = 'index'

# Use ReadTheDocs theme for sidebar navigation
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'navigation_depth': 4,
}

# Doxygen integration (Breathe + Exhale)
_doxygen_dir = os.path.abspath(os.path.join('..', 'cwipc_util', 'doxygen'))
_doxygen_xml_dir = os.path.join(_doxygen_dir, 'xml')

if True or not os.path.isdir(_doxygen_xml_dir):
    doxyfile_path = os.path.join(_doxygen_dir, 'Doxyfile')
    if os.path.isfile(doxyfile_path):
        subprocess.run(['doxygen', 'Doxyfile'], cwd=_doxygen_dir, check=False)

breathe_projects = {
    'cwipc': _doxygen_xml_dir,
}
breathe_default_project = 'cwipc'

exhale_args = {
    'containmentFolder': './api/cpp',
    'rootFileName': 'index.rst',
    'rootFileTitle': 'C++ API Reference',
    'doxygenStripFromPath': os.path.abspath('..'),
    'createTreeView': True,
}
