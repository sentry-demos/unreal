"""
Test to verify the fix for UNREAL-5: quantities defined before use
"""
import unittest
from unittest.mock import patch, MagicMock
import sys
import os

class TestCheckoutFix(unittest.TestCase):
    """Test that quantities variable is defined before being referenced"""
    
    def test_quantities_defined_before_use(self):
        """
        Verify that in the checkout function, quantities is assigned
        before it's used in len() check.
        
        This test reads the source code and checks the line order.
        """
        # Read the main.py file
        main_py_path = os.path.join(os.path.dirname(__file__), '..', 'main.py')
        with open(main_py_path, 'r') as f:
            lines = f.readlines()
        
        # Find the checkout function
        in_checkout = False
        quantities_assignment_line = None
        quantities_len_check_line = None
        
        for i, line in enumerate(lines, start=1):
            if 'def checkout()' in line:
                in_checkout = True
                continue
            
            if in_checkout:
                # Look for the quantities assignment
                if 'quantities = {int(k): v for k' in line:
                    quantities_assignment_line = i
                
                # Look for the len(quantities) check
                if 'len(quantities)' in line and 'if' in line:
                    quantities_len_check_line = i
                
                # If we found both, we can stop
                if quantities_assignment_line and quantities_len_check_line:
                    break
                
                # Stop if we reach the next function
                if line.strip().startswith('def ') and 'def checkout()' not in line:
                    break
        
        # Assert that both were found
        self.assertIsNotNone(quantities_assignment_line, 
                           "Could not find quantities assignment")
        self.assertIsNotNone(quantities_len_check_line, 
                           "Could not find len(quantities) check")
        
        # Assert that assignment comes before the check
        self.assertLess(quantities_assignment_line, quantities_len_check_line,
                       f"Bug detected: quantities used at line {quantities_len_check_line} "
                       f"before assignment at line {quantities_assignment_line}")
        
        print(f"✓ Fix verified: quantities assigned at line {quantities_assignment_line}, "
              f"used at line {quantities_len_check_line}")


if __name__ == '__main__':
    unittest.main()
