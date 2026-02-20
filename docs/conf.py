# Configuration file for the Sphinx documentation builder.

import os
import sys
sys.path.insert(0, os.path.abspath('..'))

project = 'cwipc'
copyright = '2026, CWI'

author = 'CWI DIS'

extensions = [
    'myst_parser',
    'sphinx_rtd_theme',
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
