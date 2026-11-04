import json
import operator
import os
import logging
import sentry_sdk
import sqlalchemy
from sqlalchemy import create_engine, text
from .utils import weighter
from dotenv import load_dotenv
load_dotenv()

HOST = os.environ["DB_HOST"]
DATABASE = os.environ["DB_DATABASE"]
USERNAME = os.environ["DB_USERNAME"]
PASSWORD = os.environ["DB_PASSWORD"]
FLASK_ENVIRONMENT = os.environ["FLASK_ENVIRONMENT"]
PRODUCTS_NUM = 4

class DatabaseConnectionError (Exception):
    pass


if FLASK_ENVIRONMENT == "local":
    print("> ENVIRONMENT local ")
    db = create_engine(sqlalchemy.engine.url.URL.create(
        drivername='postgresql',
        username=USERNAME,
        password=PASSWORD,
        host=HOST,
        port=5432,
        database=DATABASE
    ))
else:
    CLOUD_SQL_CONNECTION_NAME = os.environ["DB_CLOUD_SQL_CONNECTION_NAME"]
    print("> ENVIRONMENT production ")
    print("> CLOUD_SQL_CONNECTION_NAME", CLOUD_SQL_CONNECTION_NAME)
    db = sqlalchemy.create_engine(
        sqlalchemy.engine.url.URL(
            drivername='postgresql+pg8000',
            username=USERNAME,
            password=PASSWORD,
            database=DATABASE,
            query={
                'unix_sock': '/cloudsql/{}/.s.PGSQL.5432'.format(CLOUD_SQL_CONNECTION_NAME)
            }
        )
    )

# N+1 because a sql query for every product n
@sentry_sdk.trace
def get_products():
    results = []
    try:
        with sentry_sdk.start_span(name="get_products", op="db.connect"):
            connection = db.connect()

        n = weighter(operator.le, 12)
        # adjust by number of products to get the same timeout as we had in the past
        # before pg_sleep() was moved out of SELECT clause.
        n *= PRODUCTS_NUM
        query = text("SELECT * FROM products WHERE id IN (SELECT id from products, pg_sleep(:sleep_duration))")
        products = connection.execute(query, sleep_duration=n).fetchall()

        for product in products:
            # product_bundles is a "sleepy view", run the following query to get current sleep duration:
            # SELECT pg_get_viewdef('product_bundles', true)
            query = text("SELECT * FROM reviews, product_bundles WHERE productId = :x")
            reviews = connection.execute(query, x=product.id).fetchall()

            result = dict(product)
            result["reviews"] = []

            for review in reviews:
                result["reviews"].append(dict(review))
            results.append(result)

        with sentry_sdk.start_span(name="get_products.combined_reviews.json", op="serialization"):
            result = json.dumps(results, default=str)
        return result
    except Exception as err:
        raise DatabaseConnectionError('get_products') from err

# 2 sql queries max, then sort in memory
@sentry_sdk.trace
def get_products_join():
    results = []
    try:
        with sentry_sdk.start_span(name="get_products_join", op="db.connect"):
            connection = db.connect()

        with sentry_sdk.start_span(name="get_products_join", op="db.query") as span:
            products = connection.execute(
                "SELECT * FROM products"
            ).fetchall()
            span.set_tag("totalProducts",len(products))
            span.set_data("products",products)

        with sentry_sdk.start_span(name="get_products_join.reviews", op="db.query") as span:
            reviews = connection.execute(
                "SELECT reviews.id, products.id AS productid, reviews.rating, reviews.customerId, reviews.description, reviews.created FROM reviews INNER JOIN products ON reviews.productId = products.id"
            ).fetchall()
            span.set_data("reviews",reviews)
    except Exception as err:
        raise DatabaseConnectionError('get_products_join') from err

    with sentry_sdk.start_span(name="get_products_join.format_results", op="code.block") as span:
        for product in products:
            result = dict(product)
            result["reviews"] = []

            for review in reviews:
                productId=review[1]
                if productId == product["id"]:
                    result["reviews"].append(dict(review))
            results.append(result)
        span.set_data("results", results)

    with sentry_sdk.start_span(name="get_products_join.json", op="serialization"):
        result = json.dumps(results, default=str)

    return result

@sentry_sdk.trace
def get_inventory(cart):
    print("> get_inventory")

    quantities = cart['quantities']

    print("> quantities", quantities)

    productIds = []
    for productId_str in quantities.keys():
        logging.info(f"Processing product ID: {productId_str}")
        productIds.append(int(productId_str))

    print("> productIds", productIds)

    try:
        with sentry_sdk.start_span(name="get_inventory", op="db.connect"):
            connection = db.connect()
        with sentry_sdk.start_span(name="get_inventory", op="db.query") as span:
            # Use parameterized query with ANY() to safely handle array of product IDs
            query = text("SELECT * FROM inventory WHERE productId = ANY(:product_ids)")
            inventory = connection.execute(query, product_ids=productIds).fetchall()
            span.set_data("inventory",inventory)
    except Exception as err:
        raise DatabaseConnectionError('get_inventory') from err

    return inventory

def decrement_inventory(id, count):
    pass

@sentry_sdk.trace
def get_promo_code(code):
    """Get promo code from database by code string"""
    try:
        with sentry_sdk.start_span(name="get_promo_code", op="db.connect"):
            connection = db.connect()
        
        with sentry_sdk.start_span(name="get_promo_code", op="db.query") as span:
            query = text("SELECT * FROM promo_codes WHERE code = :code AND is_active = true")
            result = connection.execute(query, code=code).fetchone()
            span.set_data("promo_code", result)
        
        return result
    except Exception as err:
        raise DatabaseConnectionError('get_promo_code') from err
