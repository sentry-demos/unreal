#!/bin/bash

# Start the Gunicorn server for Flask
gunicorn -b :$PORT -w 2 --timeout 60 src.main:app &

# Start the Celery worker
celery -A src.queues.email_subscribe worker -l INFO
