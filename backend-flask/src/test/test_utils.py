# from ..utils import *

import unittest
from unittest.mock import patch, Mock
from ..utils import weighter, parseHeaders, get_iterator, yuval, chris
from datetime import datetime
from pytz import timezone

class TestFunctions(unittest.TestCase):

    # @patch('utils.datetime')
    # def test_weighter_condition_true(self, mock_datetime):
    #     mock_datetime.now.return_value = datetime(2024, 8, 22, 14, 0, 0, tzinfo=timezone('America/Los_Angeles'))
    #     condition = lambda x, y: x >= y
        
    #     result = weighter(condition, 14)
    #     # self.assertIn(result, [0.0125, 0.0625, 0.125, 0.1875, 0.25, 0.3125])

    # @patch('utils.datetime')
    # def test_weighter_condition_false(self, mock_datetime):
    #     mock_datetime.now.return_value = datetime(2024, 8, 22, 14, 0, 0, tzinfo=timezone('America/Los_Angeles'))
    #     condition = lambda x, y: x < y
        
    #     result = weighter(condition, 14)
    #     # self.assertIn(result, [0.0125, 0.0625, 0.125, 0.1875, 0.25, 0.3125])

    def test_parseHeaders_with_valid_keys(self):
        keys = ['Authorization', 'Content-Type']
        headers = {'Authorization': 'Bearer token', 'Content-Type': 'application/json'}
        
        result = parseHeaders(keys, headers)
        expected = {'Authorization': 'Bearer token', 'Content-Type': 'application/json'}
        
        self.assertEqual(result, expected)

    def test_parseHeaders_with_undefined(self):
        keys = ['Authorization', 'Content-Type']
        headers = {'Authorization': 'Bearer token', 'Content-Type': 'undefined'}
        
        result = parseHeaders(keys, headers)
        expected = {'Authorization': 'Bearer token', 'Content-Type': None}
        
        self.assertEqual(result, expected)

    def test_get_iterator_negative(self):
        result = get_iterator(-1)
        # self.assertEqual(result, None)
        # self.assertIn("Incorrect input", log.output[0])

    def test_get_iterator_zero(self):
        result = get_iterator(0)
        self.assertEqual(result, 0)

    def test_get_iterator_one(self):
        result = get_iterator(1)
        self.assertEqual(result, 1)

    def test_get_iterator_five(self):
        result = get_iterator(5)
        self.assertEqual(result, 5)

    def test_yuval(self):
        result = yuval('some text')
        self.assertEqual(result, '')

    def test_chris(self):
        result = chris()
        self.assertEqual(result, '')

if __name__ == '__main__':
    unittest.main()
