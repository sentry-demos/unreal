### Couldn't get this test to work. Leaving commented out for now.
### Requires more packages to be installed, as well as proper/more mocking

# import unittest
# from unittest.mock import patch, MagicMock
# import json

# import os
# os.environ['DB_HOST'] = 'localhost'
# os.environ['DB_DATABASE'] = 'test_db'
# os.environ['DB_USERNAME'] = 'test_user'
# os.environ['DB_PASSWORD'] = 'test_password'
# os.environ['FLASK_ENVIRONMENT'] = 'local'

# from ..db import get_products, get_products_join, get_inventory, formatArray, DatabaseConnectionError

# class TestDBFunctions(unittest.TestCase):

#     # @patch.dict('os.environ', {
#     #     'DB_HOST': 'localhost',
#     #     'DB_DATABASE': 'test_db',
#     #     'DB_USERNAME': 'test_user',
#     #     'DB_PASSWORD': 'test_password',
#     #     'FLASK_ENVIRONMENT': 'local',
#     # })
#     # @patch('..db.db.connect')
#     # @patch('..db.weighter')
#     # def test_get_products_success(self, mock_weighter, mock_db_connect):
#     #     # Mocking weighter function to return a specific value
#     #     mock_weighter.return_value = 0.1

#     #     # Mocking the database connection and queries
#     #     mock_connection = mock_db_connect.return_value.__enter__.return_value
#     #     mock_connection.execute.side_effect = [
#     #         [MagicMock(id=1, name="Product 1")],  # products query result
#     #         [MagicMock(id=1, productId=1, rating=5, customerId=1, description="Good", created="2024-08-22")]  # reviews query result
#     #     ]
        
#     #     result = get_products()
#     #     expected_result = json.dumps([{
#     #         'id': 1,
#     #         'name': 'Product 1',
#     #         'reviews': [{'id': 1, 'productId': 1, 'rating': 5, 'customerId': 1, 'description': 'Good', 'created': '2024-08-22'}]
#     #     }], default=str)

#     #     self.assertEqual(result, expected_result)

#     # @patch.dict('os.environ', {
#     #     'DB_HOST': 'localhost',
#     #     'DB_DATABASE': 'test_db',
#     #     'DB_USERNAME': 'test_user',
#     #     'DB_PASSWORD': 'test_password',
#     #     'FLASK_ENVIRONMENT': 'local',
#     # })
#     # @patch('..db.db.connect')
#     # def test_get_products_join_success(self, mock_db_connect):
#     #     # Mocking the database connection and queries
#     #     mock_connection = mock_db_connect.return_value.__enter__.return_value
#     #     mock_connection.execute.side_effect = [
#     #         [MagicMock(id=1, name="Product 1")],  # products query result
#     #         [
#     #             MagicMock(id=1, productid=1, rating=5, customerId=1, description="Good", created="2024-08-22")
#     #         ]  # reviews query result
#     #     ]
        
#     #     result = get_products_join()
#     #     expected_result = json.dumps([{
#     #         'id': 1,
#     #         'name': 'Product 1',
#     #         'reviews': [{'id': 1, 'productid': 1, 'rating': 5, 'customerId': 1, 'description': 'Good', 'created': '2024-08-22'}]
#     #     }], default=str)

#     #     self.assertEqual(result, expected_result)

#     # @patch.dict('os.environ', {
#     #     'DB_HOST': 'localhost',
#     #     'DB_DATABASE': 'test_db',
#     #     'DB_USERNAME': 'test_user',
#     #     'DB_PASSWORD': 'test_password',
#     #     'FLASK_ENVIRONMENT': 'local',
#     # })
#     # @patch('..db.db.connect')
#     # def test_get_inventory_success(self, mock_db_connect):
#     #     cart = {
#     #         'quantities': {
#     #             '1': 2,
#     #             '2': 3
#     #         }
#     #     }
#     #     expected_ids = "(1,2)"

#     #     # Mocking the database connection and query
#     #     mock_connection = mock_db_connect.return_value.__enter__.return_value
#     #     mock_connection.execute.return_value.fetchall.return_value = [
#     #         MagicMock(productId=1, quantity=10),
#     #         MagicMock(productId=2, quantity=20)
#     #     ]

#     #     result = get_inventory(cart)
#     #     expected_result = [
#     #         MagicMock(productId=1, quantity=10),
#     #         MagicMock(productId=2, quantity=20)
#     #     ]

#     #     self.assertEqual(result, expected_result)
#     #     mock_connection.execute.assert_called_with(f"SELECT * FROM inventory WHERE productId in {expected_ids}")

#     def test_formatArray(self):
#         ids = ['1', '2', '3']
#         result = formatArray(ids)
#         self.assertEqual(result, "(1,2,3)")

#     # @patch.dict('os.environ', {
#     #     'DB_HOST': 'localhost',
#     #     'DB_DATABASE': 'test_db',
#     #     'DB_USERNAME': 'test_user',
#     #     'DB_PASSWORD': 'test_password',
#     #     'FLASK_ENVIRONMENT': 'local',
#     # })
#     # @patch('..db.connect')
#     # def test_get_products_database_connection_error(self, mock_db_connect):
#     #     mock_db_connect.side_effect = BrokenPipeError
#     #     with self.assertRaises(DatabaseConnectionError) as context:
#     #         get_products()
#     #     self.assertEqual(str(context.exception), 'get_products')

#     # @patch.dict('os.environ', {
#     #     'DB_HOST': 'localhost',
#     #     'DB_DATABASE': 'test_db',
#     #     'DB_USERNAME': 'test_user',
#     #     'DB_PASSWORD': 'test_password',
#     #     'FLASK_ENVIRONMENT': 'local',
#     # })
#     # @patch('..db.db.connect')
#     # def test_get_products_join_database_connection_error(self, mock_db_connect):
#     #     mock_db_connect.side_effect = BrokenPipeError
#     #     with self.assertRaises(DatabaseConnectionError) as context:
#     #         get_products_join()
#     #     self.assertEqual(str(context.exception), 'get_products_join')

#     # @patch.dict('os.environ', {
#     #     'DB_HOST': 'localhost',
#     #     'DB_DATABASE': 'test_db',
#     #     'DB_USERNAME': 'test_user',
#     #     'DB_PASSWORD': 'test_password',
#     #     'FLASK_ENVIRONMENT': 'local',
#     # })
#     # @patch('..db.db.connect')
#     # def test_get_inventory_database_connection_error(self, mock_db_connect):
#     #     mock_db_connect.side_effect = BrokenPipeError
#     #     cart = {
#     #         'quantities': {
#     #             '1': 2,
#     #             '2': 3
#     #         }
#     #     }
#     #     with self.assertRaises(DatabaseConnectionError) as context:
#     #         get_inventory(cart)
#     #     self.assertEqual(str(context.exception), 'get_inventory')

# if __name__ == '__main__':
#     unittest.main()
