#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from speedyxml import parse, XMLParseException

import codecs, glob, pickle, os, sys, unittest

def writeResults(fns):
	for ffn in sorted(fns):
		(path, fn) = os.path.split(ffn)
		resultFn = os.path.join(path, fn.rsplit('.', 1)[0]) + '.result'

		try:
			res = parse(codecs.open(ffn, 'rb', 'utf8').read())
		except XMLParseException as e:
			res = (False, e.args[0])
		else:
			res = (True, res)

		if 'write' in sys.argv[1:]:
			pickle.dump(res, open(resultFn, 'wb'))


class Test(unittest.TestCase):

	def test_all(self):
		for ffn in sorted(glob.glob('test/*.test')):
			print('Running %s' % (ffn,))
			(path, fn) = os.path.split(ffn)
			resultFn = os.path.join(path, fn.rsplit('.', 1)[0]) + '.result'

			try:
				with codecs.open(ffn, 'rb', 'utf8') as F:
					res = parse(F.read())
			except XMLParseException as e:
				res = (False, e.args[0])
			else:
				res = (True, res)

			with open(resultFn, 'rb') as F:
				res2 = pickle.load(F)
			self.assertEqual(res, res2)


def suite():
	alltests = unittest.TestSuite()
	alltests.addTest(unittest.makeSuite(Test))
	return alltests

if __name__=='__main__':
	if 'write' in sys.argv[1:]:
		writeResults(sys.argv[2:])
	else:
		runner = unittest.TextTestRunner()
		runner.run(suite())
