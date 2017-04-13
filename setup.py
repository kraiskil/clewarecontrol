from distutils.core import setup, Extension
srcs = ['cleware_wrap.cxx', 'USBaccessBasic.cpp', 'USBaccess.cpp']
libs = ['hidapi-libusb']
cleware_module = Extension('_cleware', sources=srcs, libraries=libs)

setup (name = 'cleware',
       version = '2.3',
       author      = "Folkert van Heusden",
       description = """Cleware library bindings for Python""",
       license='AGPLv3',
       ext_modules = [cleware_module],
       py_modules = ["cleware"],
       classifiers = [
          'License :: OSI Approved :: GNU Affero General Public License v3',
          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 2.7',
          'Programming Language :: Python :: 3',
          'Programming Language :: Python :: 3.4',
       ]
       )
