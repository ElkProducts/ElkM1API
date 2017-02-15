#!/usr/bin/env python3

from distutils.core import setup, Extension
from textwrap import dedent

elk_module = Extension(
    '_ElkM1API',
    sources=[
        'ElkC1M1Tunnel.cpp',
        'ElkM1AsciiAPI.cpp',
        'ElkM1Connection.cpp',
        'ElkM1Monitor.cpp',
        'SwigCallbacks.cpp',
        'swig.i',
    ],
    swig_opts=['-DELKM1API', '-c++', '-py3'],
    extra_compile_args=['-std=c++11'])

setup(
    name='ElkM1API',
    version='0.1',
    author='ELK PRODUCTS, INC.',
    license='MIT',
    description=dedent("""\
        ElkM1API is a wrapper for the many functions that the Elk ASCII \
        protocol that allows it to request information in a familiar \
        environment, with everything wrapped down to simple calls.\
        """),
    ext_modules=[elk_module],
    py_modules=['ElkM1API'])
