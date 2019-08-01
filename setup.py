from distutils.core import setup, Extension
setup(
    name='pylibyaml',
    version='0.1',
    ext_modules=[Extension('_ext', ['pylibyaml/_ext.c'])])
