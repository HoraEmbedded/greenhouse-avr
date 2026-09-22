#!/usr/bin/env python3
"""
Subscribe to the greenhouse MQTT topic and print incoming telemetry.
Usage:
    python mqtt_monitor.py --host <cluster>.s1.eu.hivemq.cloud \
                           --user <username> --password <password>
"""
import argparse
import json
import ssl
import sys
import paho.mqtt.client as mqtt


def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("[MQTT] connected, subscribing...")
        client.subscribe(userdata["topic"], qos=0)
    else:
        print(f"[MQTT] connection refused, rc={rc}")
        sys.exit(1)


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        print(f"[{msg.topic}] {json.dumps(payload, indent=None)}")
    except Exception as exc:
        print(f"[{msg.topic}] raw: {msg.payload!r} ({exc})")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, default=8883)
    parser.add_argument("--user", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--topic", default="serre/node01/metrics")
    args = parser.parse_args()

    client = mqtt.Client(userdata={"topic": args.topic})
    client.username_pw_set(args.user, args.password)
    client.tls_set(cert_reqs=ssl.CERT_REQUIRED)
    client.on_connect = on_connect
    client.on_message = on_message

    print(f"[MQTT] connecting to {args.host}:{args.port}...")
    client.connect(args.host, args.port, keepalive=60)
    client.loop_forever()


if __name__ == "__main__":
    main()