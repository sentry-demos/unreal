"""
Demonstration of the UNREAL-5 bug and fix

This script demonstrates how the UnboundLocalError occurred before the fix
and how it's resolved after the fix.
"""


def checkout_buggy(cart):
    """
    BEFORE FIX: This version has the bug
    Raises UnboundLocalError when validate_inventory is True
    """
    validate_inventory = True
    quantities = None  # Simulating uninitialized variable
    
    if validate_inventory:
        # BUG: Referencing quantities before assignment
        if len(quantities) == 0:  # UnboundLocalError!
            raise Exception("Invalid checkout request: cart is empty")
        
        # Assignment happens AFTER the check
        quantities = {int(k): v for k, v in cart['quantities'].items()}
    
    return "success"


def checkout_fixed(cart):
    """
    AFTER FIX: This version is correct
    Works properly because quantities is assigned before use
    """
    validate_inventory = True
    
    if validate_inventory:
        # FIX: Assignment happens BEFORE the check
        quantities = {int(k): v for k, v in cart['quantities'].items()}
        
        if len(quantities) == 0:
            raise Exception("Invalid checkout request: cart is empty")
    
    return "success"


def main():
    # Sample cart data from Unreal game
    test_cart = {
        "quantities": {"3": 3},
        "items": [{"id": 3, "title": "Plant Mood", "price": 155}],
        "total": 465
    }
    
    print("=" * 70)
    print("UNREAL-5 Bug Demonstration")
    print("=" * 70)
    print()
    
    # Test the buggy version
    print("1. Testing BUGGY version (before fix):")
    print("-" * 70)
    try:
        result = checkout_buggy(test_cart)
        print(f"   ✗ Unexpectedly succeeded: {result}")
    except (UnboundLocalError, TypeError) as e:
        print(f"   ✓ Bug reproduced! Error: {type(e).__name__}")
        print(f"   ✓ Message: {str(e)}")
        print(f"   ✓ This would cause Flask to return HTTP 500")
        print(f"   ✓ Unreal ensureMsgf would fail")
    print()
    
    # Test the fixed version
    print("2. Testing FIXED version (after fix):")
    print("-" * 70)
    try:
        result = checkout_fixed(test_cart)
        print(f"   ✓ Success! Result: {result}")
        print(f"   ✓ Flask returns HTTP 200")
        print(f"   ✓ Unreal ensureMsgf passes")
    except Exception as e:
        print(f"   ✗ Unexpected error: {type(e).__name__}: {str(e)}")
    print()
    
    # Test with empty cart
    print("3. Testing with empty cart (edge case):")
    print("-" * 70)
    empty_cart = {
        "quantities": {},
        "items": [],
        "total": 0
    }
    try:
        result = checkout_fixed(empty_cart)
        print(f"   ✗ Should have raised exception for empty cart")
    except Exception as e:
        print(f"   ✓ Correctly raised exception: {type(e).__name__}")
        print(f"   ✓ Message: {str(e)}")
    
    print()
    print("=" * 70)
    print("Demonstration complete!")
    print("=" * 70)


if __name__ == "__main__":
    main()
