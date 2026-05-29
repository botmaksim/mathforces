# MathForces

MathForces — desktop-платформа для математических контестов на C++/Qt: контесты, архив задач, отправка решений, AI-проверка, рейтинги, профили, друзья, блоги и админ-панель.

## Что улучшено в этой версии

- Пересобрана структура проекта по папкам: `client/src`, `client/tabs`, `client/dialogs`, `client/ui`, `client/network`, `server/src`, `assets`, `tools`, `docs`.
- Интерфейс оформлен в более аккуратном стиле, вдохновлённом Codeforces, но с фирменным MathForces-видом.
- Добавлены светлая и тёмная темы с переключателем в верхней панели.
- Добавлены иконки вкладок.
- Добавлена стартовая welcome-страница, пока контест не выбран.
- Улучшена адаптивность: вкладки прокручиваются, главное окно имеет меньший minimum size, рабочая зона задачи переключает splitters на вертикальный режим на маленьких экранах.
- Таблицы получили сортировку и поиск.
- Архив задач теперь открывает полноценную карточку задачи: условие, метаданные, PDF/Typst предпросмотр, отправка решения и список посылок.
- Добавлены non-blocking уведомления об успешной отправке решения и других действиях.
- Улучшена обработка сетевых ошибок: показываются понятные сообщения и JSON-ошибки сервера.
- Убрана runtime-загрузка Typst через `npx -y typst`: сервер ищет локальный компилятор в `tools/typst` или через `MATHFORCES_TYPST_BIN`.

## Структура

```text
MathForces/
  client/
    src/          # точка входа клиента
    config/       # клиентская конфигурация API
    tabs/         # основные вкладки
    dialogs/      # модальные окна
    ui/           # темы, toast, таблицы, подсветка, welcome
    network/      # обработка сетевых ошибок
  server/
    src/          # API, БД, auth, LLM, SMTP
  sql/            # схема БД
  assets/brand/   # логотипы MathForces
  tools/typst/    # локальный Typst CLI
  docs/           # заметки по архитектуре
```

Подробности по архитектуре: `docs/ARCHITECTURE.md`.

## Зависимости Debian/Ubuntu

```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev qt6-base-dev-tools \
  qt6-httpserver-dev qt6-websockets-dev qt6-pdf-dev libqt6sql6-psql \
  postgresql postgresql-client libxkbcommon-dev
```

Typst больше не докачивается сервером через `npx`. Положите официальный бинарник в папку:

```text
tools/typst/typst      # Linux/macOS
tools/typst/typst.exe  # Windows
```

или укажите путь переменной окружения:

```bash
export MATHFORCES_TYPST_BIN=/absolute/path/to/typst
```

Если Typst установлен в системе и доступен через `PATH`, сервер тоже его найдёт.

## PostgreSQL

Запуск службы:

```bash
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

Создание пользователя и базы:

```bash
sudo -u postgres psql -c "CREATE USER mathforces WITH PASSWORD 'mathforces_pass';"
sudo -u postgres psql -c "CREATE DATABASE mathforces_db OWNER mathforces;"
```

Инициализация схемы:

```bash
cp sql/init_db.sql /tmp/init_db.sql
sudo -u postgres psql -d mathforces_db -f /tmp/init_db.sql
rm /tmp/init_db.sql
```

Если база уже существовала, обновите права:

```bash
sudo -u postgres psql -d mathforces_db -c "GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO mathforces; GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO mathforces;"
```

## Конфигурация

```bash
cp config.env.example config.env
```

Отредактируйте `config.env`: порт сервера, данные БД, SMTP и `OPENROUTER_API_KEY`.

## Сборка

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j$(nproc)
```

## Запуск

Сервер:

```bash
./server/Server
```

Клиент:

```bash
./client/Client
```

## Тестовые аккаунты

Вход выполняется по email:

- Super Admin: `superadmin@example.com` / `12345`
- Администратор: `admin@example.com` / `12345`
- Студент: `student@example.com` / `12345`

Если база была создана старой версией, можно обновить email тестовых пользователей:

```bash
sudo -u postgres psql -d mathforces_db -c "UPDATE users SET email = 'student@example.com' WHERE username = 'student';"
sudo -u postgres psql -d mathforces_db -c "UPDATE users SET email = 'admin@example.com' WHERE username = 'admin';"
sudo -u postgres psql -d mathforces_db -c "UPDATE users SET email = 'superadmin@example.com' WHERE username = 'superadmin';"
```

## OAuth

Для Google OAuth в Google Cloud Console укажите:

- Authorized JavaScript origins: `http://127.0.0.1:8080`
- Authorized redirect URIs: `http://127.0.0.1:8080/api/oauth_callback_client`

## Примечание по проверке

В среде сборки этого архива не было Qt/PostgreSQL и не было доступа контейнера к GitHub для скачивания официального Typst binary. Код переведён на локальный Typst и структура проекта подготовлена; для полной runtime-проверки нужно положить официальный `typst` в `tools/typst` и собрать проект на машине с Qt6/PostgreSQL.
