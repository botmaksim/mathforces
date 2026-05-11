# Mathforces

Система для проведения математических контестов (серверная и клиентская часть на C++ Qt). 

## 1. Установка зависимостей (Debian Trixie)

Установите необходимые пакеты, включая компиляторы, утилиты сборки, компоненты Qt6 и PostgreSQL:

```bash
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-httpserver-dev \
qt6-websockets-dev libqt6sql6-psql postgresql postgresql-client
```

## 2. Настройка PostgreSQL

### 2.1. Запуск службы
Если вы столкнулись с ошибкой "*connection to server on socket ... failed*", значит сервер баз данных не запущен. Запустите его и добавьте в автозагрузку:

```bash
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

### 2.2. Создание пользователя и базы данных
Выполните команды от имени системного пользователя `postgres`, чтобы создать пользователя `mathforces` и привязать к нему базу `mathforces_db`:

```bash
sudo -u postgres psql -c "CREATE USER mathforces WITH PASSWORD 'mathforces_pass';"
sudo -u postgres psql -c "CREATE DATABASE mathforces_db OWNER mathforces;"
```

### 2.3. Инициализация таблиц
Пользователь `postgres` не имеет прав на чтение содержимого вашей домашней директории (`/home/username/...`), из-за чего возникает ошибка "*Permission denied*". 

Чтобы этого избежать, скопируйте дамп в общую временную папку `/tmp`, примените его и затем удалите исходник:

```bash
cp sql/init_db.sql /tmp/init_db.sql
sudo -u postgres psql -d mathforces_db -f /tmp/init_db.sql
rm /tmp/init_db.sql
```
*(Примечание: сообщения `ЗАМЕЧАНИЕ: таблица ... не существует, пропускается` — это нормальное поведение файла `init_db.sql` при первой установке).*

Если вы уже выполнили инициализацию до обновления, выдайте права пользователю вручную:
```bash
sudo -u postgres psql -d mathforces_db -c "GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO mathforces; GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO mathforces;"
```

## 3. Настройка конфигурации

Скопируйте шаблон переменных окружения:

```bash
cp config.env.example config.env
```

Отредактируйте файл `config.env` (например, через `nano config.env`). Вставьте ваш реальный API-ключ от OpenRouter в поле `OPENROUTER_API_KEY`.

## 4. Сборка проекта

Находясь в корневом каталоге проекта `mathforces`, выполните команды сборки:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

## 5. Запуск

Все исполняемые файлы будут лежать в папке `build` (разложенные по подпапкам `server` и `client`):

1. В первом окне терминала (находясь в папке `build`), запустите бэкенд-сервер:
   ```bash
   ./server/Server
   ```

2. Во втором окне терминала (также из папки `build`), запустите графический клиент:
   ```bash
   ./client/Client
   ```

### Тестовые аккаунты
Инициализационный скрипт базы данных уже создал два аккаунта:
* **Администратор**: логин `admin` / пароль `12345`
* **Студент**: логин `student` / пароль `12345`

*Подсказка: Текстовые поля с условиями задач и блоками для решений поддерживают подсветку синтаксиса встроенного математического текста (Latex и Typst).*
