#!/usr/bin/python

import os
import shutil

from setuptools import setup
from setuptools.command.install import install

from distutils.core import Extension

with open("README", "r", encoding="utf-8") as fh:
	long_description = fh.read()


class CustomInstallCommand(install):
    def run(self):
        # Run the standard install process
        super().run()

        # After installation, find where the .so was installed and copy the .pyi there
        for root, dirs, files in os.walk(self.install_lib):
            for file in files:
                if file.startswith('speedyxml') and file.endswith(('.so', '.pyd')):
                    so_path = os.path.join(root, file)
                    dest_dir = os.path.dirname(so_path)
                    shutil.copyfile('src/speedyxml.pyi', os.path.join(dest_dir, 'speedyxml.pyi'))
                    print(f'Copied speedyxml.pyi to {dest_dir}')
                    return


setup(
	name				=	'speedyxml',
	version				=	'0.4.0.15',
	description			=	'Speedy XML parser for Python',
	author				=	'kilroy',
	author_email		=	'kilroy@81818.de',
	license				=	'LGPL',
	py_modules			=	[],
	ext_modules			=	[
        Extension('speedyxml', ['src/speedyxml.c'])
	],
	test_suite			=	'test.test.suite',
	classifiers			=	[
		'Development Status :: 4 - Beta',
		'Intended Audience :: Developers',
		'Natural Language :: English',
		'Operating System :: POSIX',
		'Programming Language :: Python',
		'Programming Language :: Python :: 3',
		'Topic :: Software Development :: Libraries :: Python Modules',
		'Topic :: Text Processing :: Markup :: XML',
		'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
	],
    long_description=long_description,
    long_description_content_type='text/plain',
    cmdclass={
        'install': CustomInstallCommand,
    },
)
