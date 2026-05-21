# Mathforces (Архитектура на базе userver + Qt6)

Система для проведения математических соревнований (контестов), построенная на гибридном высокопроизводительном стеке.

---

## 1. Архитектурный обзор

Проект разделен на две независимые части, взаимодействующие по протоколу HTTP REST API с обменом данными в формате JSON. Прямой доступ клиента к базе данных сервера полностью исключен.

### 1.1 Серверная часть (C++ / userver / PostgreSQL)
- **Фреймворк**: Асинхронный микросервисный фреймворк `userver` (разработка Яндекс), обеспечивающий высокую пропускную способность за счет корутин (tasks).
- **СУБД**: Основная распределенная СУБД проекта — **PostgreSQL**.
- **Драйвер БД**: Работа с БД осуществляется исключительно через асинхронный неблокирующий компонент `userver::storages::postgres`.
- **Авторизация**: Реализована безопасная JWT-авторизация (JSON Web Tokens). Секретный ключ подписи JWT загружается из переменной окружения `JWT_SECRET` (со скрытием в `config.env` / `.env` на продакшене), а алгоритм HMAC-SHA256 реализован с использованием средств шифрования `userver` и OpenSSL.

### 1.2 Клиентская часть (C++ / Qt6 / SQLite)
- **Паттерн MVC/MVP**: 
  - Разделение ответственности: графические представления (View) полностью изолированы от логики запросов и сетевых вызовов.
  - Весь трафик и общение с сервером инкапсулированы в отдельном шлюзе сетевых сервисов-репозиториев (`ApiClient`/`Repository`), находящимся в слое **Model**.
  - Навигация и трансляция данных осуществляются через презентеры (`Presenters`).
- **Списки и таблицы данных**: Для связывания динамических списков, таблиц результатов, рейтингов и архивов задач используются структурированные наследники класса `QAbstractTableModel` и `QAbstractListModel` во избежание ручного обновления UI.
- **Локальный кэш (Qt SQL)**:
  - Клиент укомплектован локальной СУБД **SQLite** для обеспечения бесперебойного кэширования справочников контестов, рейтингов участников, локальных настроек пользователя и автосохранения текстовых черновиков решений задач во время непредвиденного дисконнекта.
  - Создание локальных таблиц и миграции базы данных происходят автоматически при первом запуске клиента. Прямые SQL-запросы вынесены исключительно в специализированный класс слоя моделей (`LocalDb`).

---

## 2. Предварительные требования и пакеты

```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-httpserver-dev \
qt6-websockets-dev libqt6sql6-psql postgresql postgresql-client typst qt6-pdf-dev libxkbcommon-dev
```

Для успешной компиляции и запуска всех компонентов системы вам потребуются следующие зависимости в вашей системе (пример для ОС Linux Ubuntu 22.04 LTS / 24.04 LTS):

### 2.1 Системные зависимости компилятора и утилит
```bash
sudo apt update
sudo apt install -y build-essential cmake ccache ninja-build git tar unzip curl wget g++-12
```

### 2.2 Зависимости для Qt6 Клиента
```bash
sudo apt install -y qt6-base-dev qt6-declarative-dev libqt6sql6-sqlite
```

### 2.3 Зависимости для userver Сервера (и СУБД)
Асинхронный фреймворк `userver` активно использует библиотеки Boost, компилятор Protobuf и инструменты среды Python для тестовых утилит (даже со статусом OFF), поэтому перед его сборкой необходимо обязательно установить пакет заголовков Boost, `virtualenv` и компоненты Protobuf. Также для сетевых операций, асинхронного DNS, работы с HTTP и PostgreSQL требуются соответствующие девелоперские библиотеки.

В зависимости от вашей версии операционной системы (например, Debian 12 / Ubuntu 24.04), установите метапакет `postgresql-server-dev-all` либо версию, соответствующую вашей СУБД (например, `postgresql-server-dev-15` или `postgresql-server-dev-16`):

```bash
# Установка общих зависимостей userver и PostgreSQL
sudo apt install -y libssl-dev libpq-dev postgresql-client libboost-all-dev virtualenv protobuf-compiler libprotobuf-dev python3-protobuf libyaml-cpp-dev libjemalloc-dev libhttp-parser-dev libev-dev libmongoc-dev postgresql-server-dev-all libc-ares-dev libcurl4-openssl-dev zlib1g-dev pkg-config libhiredis-dev libgrpc-dev libgrpc++-dev protobuf-compiler-grpc
```

*Примечание*: **Сам фреймворк `userver` не требует ручной установки!** В проекте настроен автоматический умный fallback в `server/CMakeLists.txt`. Если в вашей операционной системе не предустановлен системный пакет `userver`, CMake самостоятельно скачает стабильный релиз через компонент `FetchContent` напрямую из официального репозитория на GitHub и скомпилирует его локально вместе с сервером. Чтобы компиляция прошла максимально быстро, сборка внутренних юнит-тестов и сэмплов самого userver автоматически отключается (`USERVER_BUILD_TESTS=OFF`, `USERVER_BUILD_SAMPLES=OFF`).

#### ⚠️ Решение ошибки компиляции `Could NOT find Boost (missing: Boost_INCLUDE_DIR)`
Если компилятор прерывает выполнение CMake с сообщением:
```
Could NOT find Boost (missing: Boost_INCLUDE_DIR)
```
это означает, что в операционной системе не установлена библиотека Boost (или отсутствуют её заголовочные файлы сборщика C++).

1. Установите Boost:
   ```bash
   sudo apt update
   sudo apt install -y libboost-all-dev
   ```
2. **Критически важно (Сброс кэша CMake!):** Поскольку CMake кэширует результаты неудачного поиска в файле `CMakeCache.txt`, простая повторная попытка запуска `cmake` в существующей папке `build` выдаст ту же ошибку. Вам необходимо полностью удалить старую папку билда и собрать всё заново:
   ```bash
   # Выйдите в корень проекта, удалите папку build и сконфигурируйте cmake с нуля:
   cd /mathforces
   rm -rf build && mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```

#### ⚠️ Решение ошибки компиляции `No virtualenv binary found`
Если вы видите ошибку:
```
No virtualenv binary found, try to install:
Debian: sudo apt install virtualenv
```
это означает, что сборочные скрипты userver не могут найти утилиту изолированных окружений Python (`virtualenv`).

Для её решения установите пакет и сбросьте кэш CMake:
1. Установите пакет:
   ```bash
   sudo apt update
   sudo apt install -y virtualenv
   ```
2. Сбросьте сборочный кэш CMake и запустите конфигурацию заново:
   ```bash
   cd /mathforces
   rm -rf build && mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```

#### ⚠️ Решение ошибки компиляции `userver failed to find Protobuf compiler`
Если вы видите ошибку:
```
Could NOT find Protobuf (missing: Protobuf_LIBRARIES Protobuf_INCLUDE_DIR)
CMake Error at build/_deps/userver-src/cmake/SetupProtobuf.cmake:7 (message):
  userver failed to find Protobuf compiler.
```
это означает, что сборочные скрипты userver требуют установленный компилятор общего формата данных Google Protobuf.

Для её решения установите компилятор Protobuf и сбросьте кэш CMake:
1. Установите пакеты Protobuf:
   ```bash
   sudo apt update
   sudo apt install -y protobuf-compiler libprotobuf-dev python3-protobuf
   ```
2. Сбросьте сборочный кэш CMake и запустите конфигурацию заново:
   ```bash
   cd /mathforces
   rm -rf build && mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```

#### ⚠️ Решение ошибки компиляции `Could not find PostgreSQLInternal package`
Если при запуске конфигурации CMake выдает следующую ошибку поиска библиотек СУБД PostgreSQL:
```
Could not find `PostgreSQLInternal` package.
      Debian: sudo apt update && sudo apt install libpq-dev postgresql-server-dev-12
 ...
 (missing: PostgreSQLInternal_LIBRARIES)
```
это означает, что сборочной системе `userver` необходим установленный пакет заголовков PostgreSQL Server (`server-dev`), версия которого строго согласуется с установленной у вас в системе версией СУБД.

> **Обратите внимание**: Ошибка в CMake настойчиво предлагает установить именно `postgresql-server-dev-12`. Это рекомендация из захардкоженных шаблонов поиска самого фреймворка `userver`. На современных ОС (например, Debian 12 имеет PostgreSQL 15, Ubuntu 22.04 — PostgreSQL 14, а Ubuntu 24.04 — PostgreSQL 16) пакет `postgresql-server-dev-12` отсутствует в репозиториях, что приводит к ошибке: `Unable to locate package postgresql-server-dev-12`.

**Правильное решение проблемы:**

1. **Вариант А (Автоматический выбор - наиболее стабильный):** Установите универсальный метапакет, который автоматически загрузит девелоперские библиотеки под текущие доступные версии PostgreSQL на вашем дистрибутиве:
   ```bash
   sudo apt update
   sudo apt install -y postgresql-server-dev-all
   ```
2. **Вариант Б (Установка точечной версии под вашу СУБД):** Проверьте, какая версия PostgreSQL используется в вашей системе, вызвав команду:
   ```bash
   psql --version
   # или
   pg_config --version
   ```
   Если вывод содержит, к примеру, версию `15.x`, установите строго соответствующий пакет разработчика:
   ```bash
   # Для PostgreSQL 15:
   sudo apt install -y postgresql-server-dev-15

   # Для PostgreSQL 16:
   sudo apt install -y postgresql-server-dev-16

   # Для PostgreSQL 17:
   sudo apt install -y postgresql-server-dev-17
   ```
3. **Сбросьте сборочный кэш CMake:** Чтобы обновления пакетов применились, удалите поврежденный сборочный кэш и запустите компиляцию с чистого листа:
   ```bash
   cd /mathforces
   rm -rf build && mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```

#### ⚠️ Решение ошибки компиляции `Error while compiling libpq patch`
Если конфигурация прерывается на сборке патча для библиотеке `libpq`:
```
error: too few arguments to function ‘pqCommandQueueAdvance’
...
If there are errors up above then the versions of libpq, libpgport and libpgcommon diverged
...
or disable libpq patching via CMake flag -DUSERVER_FEATURE_PATCH_LIBPQ=OFF.
```
Эта ошибка возникает из-за расхождения внутренних сигнатур функций в заголовках `libpq-int.h` (например, в PostgreSQL 15 на Debian 12 / Ubuntu) с тем, что ожидает userver версии 1.0.0 (в частности, для `pqCommandQueueAdvance`).

**Решение проблемы:**

Мы уже внедрили автоматическое отключение патча `libpq` непосредственно в `server/CMakeLists.txt` с помощью флага `set(USERVER_FEATURE_PATCH_LIBPQ OFF CACHE BOOL "" FORCE)`.

Чтобы применить эти настройки, сделайте полный сброс кэша CMake:
```bash
# Очистите поврежденный кэш старой конфигурации:
cd /mathforces
rm -rf build && mkdir build && cd build

# Сконфигурируйте заново (флаг отключения патчинга применится автоматически из CMakeLists):
cmake -DCMAKE_BUILD_TYPE=Release ..
```

*При желании вы также можете явно передать этот флаг через консоль:*
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DUSERVER_FEATURE_PATCH_LIBPQ=OFF ..
```

#### ⚠️ Решение ошибки компиляции `grpc_cpp_plugin does not exist`
Если при запуске CMake вы видите ошибку:
```
The imported target "gRPC::grpc_cpp_plugin" references the file
   "/usr/bin/grpc_cpp_plugin"
but this file does not exist.
```
Это происходит потому, что в Debian 12 / Ubuntu CMake-файлы пакета `libgrpc-dev` содержат ссылки на grpc-плагины для протобуфа, но сами плагины (бинарные файлы вроде `grpc_cpp_plugin`) по умолчанию не поставляются с основным пакетом разработчика и вынесены в обособленный системный репозиторий.

**Решение проблемы:**

Установите пакет grpc-компиляторов и плагинов для Protocol Buffers:
```bash
sudo apt update
sudo apt install -y protobuf-compiler-grpc
```

После установки, полностью сбросьте кэш CMake и запустите конфигурацию заново:
```bash
cd /mathforces
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

---

## 3. Настройка окружения (Config & Secrets)

Все конфигурационные параметры сервера и клиента, включая секретные ключи, хосты и порты, жестко разграничены и не содержат заглушек в кодовых базах. Они считываются во время выполнения из файла конфигурации окружения.

### 3.1 Создание файла `config.env`
Создайте текстовый файл `config.env` в корневом каталоге проекта `/mathforces` (или скопируйте его из `.env.example`):

```ini
# --- СЕРВЕРНЫЕ НАСТРОЙКИ ---
SERVER_PORT=3000
JWT_SECRET=U3VwZXJTZWNyZXRKMTJZU2Vzc2lvbktleVRvU2lnblBheWxvYWRfYnoyNjY=

# --- НАСТРОЙКИ БАЗЫ ДАННЫХ POSTGRESQL ---
DB_HOST=127.0.0.1
DB_PORT=5432
DB_NAME=mathforces_db
DB_USER=mathforces
DB_PASS=mathforces_pass

# --- НАСТРОЙКИ КЛИЕНТА ---
# URL-адрес подключения к API сервера
CLIENT_BASE_URL=http://127.0.0.1:3000
```

---

## 4. Развертывание и Инициализация Базы Данных (PostgreSQL)

### 4.1 Запуск СУБД через Docker (рекомендуется)
Для легкого и чистого развертывания запустите контейнер PostgreSQL с параметрами из вашего `config.env`:

```bash
docker run --name mathforces-db \
  -e POSTGRES_DB=mathforces_db \
  -e POSTGRES_USER=mathforces \
  -e POSTGRES_PASSWORD=mathforces_pass \
  -p 5432:5432 \
  -d postgres:15-alpine
```

#### ⚠️ Решение частой ошибки: "bind: address already in use (5432)"
Если при запуске docker-команды вы видите ошибку вида:
`docker: Error response from daemon: driver failed programming external connectivity ... bind: address already in use`
это означает, что на вашем компьютере уже запущена локальная служба PostgreSQL, либо другой контейнер использует порт `5432`.

**Варианты решения:**

**Вариант А. Найти и остановить процессы, занимающие порт 5432:**
```bash
# Для Linux/Debian/Ubuntu:
# Найти процесс, который слушает порт 5432 (обычно это системный postgresql.service)
sudo lsof -i :5432

# Остановить системную службу postgres, чтобы освободить порт:
sudo systemctl stop postgresql
# Если вы не хотите, чтобы она запускалась автоматически при загрузке:
sudo systemctl disable postgresql
```

**Вариант Б. Изменить порт в настройках Docker и `config.env`:**
Если вы хотите сохранить работающий локальный PostgreSQL и запустить Docker-контейнер параллельно на другом свободном порту (например, `5433`):
1. Измените порт маппинга контейнера при запуске на `-p 5433:5432`:
   ```bash
   docker run --name mathforces-db \
     -e POSTGRES_DB=mathforces_db \
     -e POSTGRES_USER=mathforces \
     -e POSTGRES_PASSWORD=mathforces_pass \
     -p 5433:5432 \
     -d postgres:15-alpine
   ```
2. Обновите порт базы данных в вашем файле `config.env`:
   ```ini
   DB_PORT=5433
   ```
3. И не забудьте обновить строку подключения `dbconnection` в `server/config.yaml` на `5433`:
   ```yaml
   dbconnection: 'postgresql://mathforces:mathforces_pass@127.0.0.1:5433/mathforces_db'
   ```

### 4.2 Инициализация схем бд и таблиц
Используйте предоставленные SQL Скрипты для накатывания структуры данных. 

Подключитесь к контейнеру и примените файл миграции:
```bash
# Импортируем схемы таблиц и начальные данные БД
psql -h 127.0.0.1 -p 5432 -U mathforces -d mathforces_db -f ./sql/init_db.sql
```
*(Если вы переназначили порт на `5433`, то замените `-p 5432` на `-p 5433` соответственно).*

---

## 5. Компиляция и Сборка Проекта

Сборка осуществляется кроссплатформенным инструментом `CMake`. Вы можете скомпилировать и Сервер, и Клиент одновременно из единого сборочного каталога.

### 5.1 Сборка через Терминал
В корневом каталоге проекта выполните:

```bash
# Очищаем кэш и старые файлы конфигурации (обязательно при возникновении ошибок зависимостей!)
rm -rf build

# Создаем рабочую папку сборки
mkdir build && cd build

# Конфигурируем CMake проект в Release режиме с оптимизациями
cmake -DCMAKE_BUILD_TYPE=Release ..

# Компилируем все цели (клиент, сервер, тесты) используя все потоки процессора
make -j$(nproc)
```

После завершения компиляции в каталоге `build` появятся исполняемые файлы:
- Сервер: `build/server/mathforces_server`
- Клиент: `build/client/mathforces_client`
- Юнит-тесты клиента: `build/client/ClientTests`

---

## 6. Запуск Системы

### 6.1 Запуск Сервера
Исполняемому файлу сервера требуется доступ к файлу `config.env` и yaml-конфигурации userver. Запустите сервер из корня папки проекта, чтобы он корректно прочитал конфигурационный файл:

```bash
# Перейдите в каталог с исполняемым файлом или запустите его с указанием путей
cd /mathforces
./build/server/mathforces_server --config ./server/config.yaml
```
При запуске вы увидите инициализационные логи асинхронного движка `userver` и успешное подключение к пулу мастер-хоста PostgreSQL.

### 6.2 Запуск Клиента (Qt6)
Клиент при запуске проверяет наличие локального кэша `local_cache.db` (SQLite) и, при его отсутствии, автоматически выполняет DDL миграции для создания внутренней структуры.

Запуск клиента осуществляется командой:
```bash
./build/client/mathforces_client
```

---

## 7. Ротационное тестирование и проверка кода

В составе системы поставляется набор автоматизированных тестов для проверки функционирования локального кэша, парсинга моделей, валидации презентеров и корректности сборки.

### 7.1 Запуск тестов клиента (Qt Test)
```bash
cd build
./client/ClientTests
```

---

## 8. Использование локального кэша в офлайн-режиме

В Qt-приложении реализован умный фоновый процесс сохранения истории действий:
1. **Настройки**: Локальные параметры сессии пишутся в таблицу `settings`.
2. **Черновики (Drafts)**: При вводе ответа или кода решения в текстовый редактор активного контеста, клиент с применением debounce-эффекта (раз в 1 секунду) фоновым образом сохраняет измененную строку в таблицу `task_drafts`. При перезапуске вкладки или потере фокуса черновик восстанавливается автоматически, предохраняя данные от сетевых сбоев.
3. **Офлайн Кеш**: При просмотре таблиц результатов контестов, списков соревнований и рейтингов, последние стабильные JSON-объекты фиксируются в БД SQLite. При сбое сетевого подключения они мгновенно транслируются пользователю.

### Тестовые аккаунты
Инициализационный скрипт базы данных уже создал несколько аккаунтов (теперь вход выполняется по Email):
* **Super Admin**: email `superadmin@example.com` / пароль `12345`
* **Администратор**: email `admin@example.com` / пароль `12345`
* **Студент**: email `student@example.com` / пароль `12345`