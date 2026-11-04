FROM python:3.11-slim-bookworm

WORKDIR /src
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .

CMD ["flask", "run", "--host", "0.0.0.0", "--port", "8080"]
