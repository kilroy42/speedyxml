#!/usr/bin/env python

from setuptools import setup

from distutils.core import Extension


#from distutils.core import setup, Extension

setup(
	name				=	'speedyxml',
	version				=	'0.3.9.1',
	description			=	'Speedy XML parser for Python',
	author				=	'kilroy',
	author_email		=	'kilroy@uni-koblenz.de',
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
		'Programming Language :: Python :: 2',
		'Programming Language :: Python :: 3',
		'Topic :: Software Development :: Libraries :: Python Modules',
		'Topic :: Text Processing :: Markup :: XML',
		'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)',
	],
)
