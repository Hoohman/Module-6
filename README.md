# Сетевое взаимодействие сервисов

Два сервиса, взаимодействующих по TCP:
- **NetworkLogFilter** (отправитель) — фильтрует IPv4-адреса и отправляет результаты по сети.
- **receiver** (приёмник) — принимает JSON-сообщения и выводит их в консоль.

## Сборка

```bash
cmake -B build -G Ninja
cmake --build build
```

## Запуск

Сначала запустить приёмник, затем отправитель.

**Терминал 1 — приёмник:**
```bash
./build/receiver_service/receiver.exe
```

**Терминал 2 — отправитель:**
```bash
./build/NetworkLogFilter/NetworkLogFilter.exe
```

## Пример вывода

**Приёмник:**
```
Receiver launched on 30303 port
Received: {"source_service":"ipv4_filter","timestamp_utc":"2026-10-04 18:00:12","payload":"192.168.0.5 - GET /"}
Received: {"source_service":"ipv4_filter","timestamp_utc":"2026-10-04 18:00:12","payload":"192.168.0.200 - GET /api"}
Received: {"source_service":"ipv4_filter","timestamp_utc":"2026-10-04 18:00:12","payload":"192.168.0.50 - GET /"}
Client disconnected cleanly.
```

**Отправитель:**
```
Starting IPv4 Filter Service...
Successfully connected (127.0.0.1:30303)
```

## Обработка ошибок

- **Приёмник недоступен** — отправитель делает 3 попытки подключения с интервалом 1.5 сек, при неудаче выводит результаты локально.
- **Некорректный формат** — приёмник логирует ошибку парсинга JSON и продолжает работу.
- **Разрыв соединения** — отправитель сообщает об ошибке и корректно завершается, приёмник фиксирует отключение клиента.
