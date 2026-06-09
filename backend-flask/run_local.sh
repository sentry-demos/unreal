#!/bin/bash

set -e

# This is not a standalone script. It is called from ../deploy that
# sets up the right environemnt variables and files for it.

if [ ! -d ./venv ]; then
    python3 -m venv ./venv
fi
source venv/bin/activate
pip3 install -r requirements.txt

# Store PIDs for cleanup
CELERY_PID=""


function cleanup {
  set +e
  # Kill Celery worker by PID since it doesn't listen on a port to be detected by stop.sh
  if [ -n "$CELERY_PID" ]; then
    kill -9 $CELERY_PID 2>/dev/null
  fi

  # Kill SSH tunnel by port
  stop.sh google_compute_engine $FLASK_LOCAL_REDISPORT
  stop.sh flask $FLASK_LOCAL_PORT 
}

# Register cleanup for all possible termination signals
trap cleanup EXIT

# Set up SSH tunnel to the cloud Redis instance
# (`deploy` ensures we are authenticated with Google Cloud)
echo "Setting up SSH tunnel to Redis server at $REDIS_SERVER_IP:6379"
gcloud compute ssh redis-relay --tunnel-through-iap --zone=${GCP_REGION}-a -- -N -L $FLASK_LOCAL_REDISPORT:$REDIS_SERVER_IP:6379 &


# Give the SSH tunnel time to establish
sleep 3

# Start Celery worker
echo "Starting Celery worker..."
celery -A src.queues.email_subscribe worker -l INFO --concurrency=1 &
CELERY_PID=$!
echo "Celery worker started with PID: $CELERY_PID"

# Run Flask in the background and capture its PID
echo "Starting Flask server on port $FLASK_LOCAL_PORT"
gunicorn -b :$FLASK_LOCAL_PORT -w 2 --timeout 60 src.main:app
