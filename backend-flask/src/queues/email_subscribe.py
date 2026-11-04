from celery import Celery, signals
import sentry_sdk, os, dotenv
import os

dotenv.load_dotenv()

redis_host = os.environ.get("FLASK_REDISHOST", "localhost")
redis_port = int(os.environ.get("FLASK_LOCAL_REDISPORT", 6379))

redis_url = f"redis://{redis_host}:{redis_port}/1"

app = Celery('subscribe',
             broker=redis_url,
             backend=redis_url,
             include=['src.queues.tasks'],
             broker_connection_retry_on_startup=True)

# Set default queue name
app.conf.task_default_queue = 'celery-new-subscriptions'

# Initialize Sentry SDK on Celery startup
@signals.celeryd_init.connect
def init_sentry(**_kwargs):
    dotenv.load_dotenv()
    RELEASE = os.environ["FLASK_RELEASE"]
    DSN = os.environ["FLASK_DSN"]
    ENVIRONMENT = os.environ["FLASK_ENVIRONMENT"]
    sentry_sdk.init(
        dsn=DSN,
        release=RELEASE,
        environment=ENVIRONMENT,
        traces_sample_rate=1.0,
        profiles_sample_rate=1.0,
        enable_logs=True,
    )

if __name__ == '__main__':
    app.start()
