#!/bin/bash

# Генерация самоподписанных сертификатов для тестирования
openssl req -x509 -newkey rsa:4096 -keyout server.key -out server.crt -days 365 -nodes -subj "/CN=localhost/O=SecurityAudit/C=RU"

echo "Certificates generated:"
ls -la server.*
