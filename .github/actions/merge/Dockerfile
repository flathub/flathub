FROM python:3.12 AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libcairo2-dev libgirepository1.0-dev

ADD requirements.txt .

RUN python -m venv /venv && \
    /venv/bin/python -m pip install -r requirements.txt PyGObject==3.50.0

FROM python:3.12-slim
ENV PATH="/venv/bin:$PATH"

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        git gh libcairo2 libgirepository-1.0-1 gir1.2-json-1.0 && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

COPY --from=builder /venv /venv
COPY . /app
WORKDIR /app

CMD ["/app/entrypoint.py"]
