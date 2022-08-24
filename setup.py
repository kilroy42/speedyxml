#!/usr/bin/python

from setuptools import setup

from distutils.core import Extension

with open("README", "r", encoding="utf-8") as fh:
	long_description = fh.read()

setup(
	name				=	'speedyxml',
	version				=	'0.4.0.2',
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
)
