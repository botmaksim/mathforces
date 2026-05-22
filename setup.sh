#!/bin/bash
set -e

echo "======================================"
echo "Установка зависимостей MathForces"
echo "======================================"

# Устанавливаем зависимости системы
sudo apt update
sudo apt install -y build-essential cmake qt6-base-dev qt6-base-dev-tools qt6-httpserver-dev qt6-websockets-dev libqt6sql6-psql postgresql postgresql-client typst qt6-pdf-dev libxkbcommon-dev ccache ninja-build git tar unzip curl wget g++-12 libssl-dev libpq-dev

echo "======================================"
echo "Очистка кэша от старой архитектуры (Userver -> Qt6)"
echo "======================================"

# Очищаем старые артефакты, в которых могут быть ссылки на тяжелый userver
if [ -d "build" ]; then
    echo "Удаляем старую папку build..."
    rm -rf build
fi

# Удаляем файлы handlers.cpp/hpp, если они остались от старого проекта,
# так как они вызывают ошибки компиляции и больше не нужны.
rm -f server/handlers.cpp server/handlers.hpp 

echo "======================================"
echo "Конфигурация PostgreSQL"
echo "======================================"

sudo systemctl start postgresql
sudo systemctl enable postgresql

# Создание юзера
sudo -u postgres psql -c "DO \$\$ BEGIN
    IF NOT EXISTS (SELECT FROM pg_catalog.pg_roles WHERE rolname = 'mathforces') THEN
        CREATE ROLE mathforces LOGIN PASSWORD 'mathforces_pass';
    END IF;
END \$\$;"

# Создание базы
sudo -u postgres psql -c "SELECT 1 FROM pg_database WHERE datname='mathforces_db'" | grep -q 1 || \
sudo -u postgres psql -c "CREATE DATABASE mathforces_db OWNER mathforces;"

# Инициализация
cp sql/init_db.sql /tmp/init_db.sql
sudo -u postgres psql -d mathforces_db -f /tmp/init_db.sql
rm /tmp/init_db.sql
sudo -u postgres psql -d mathforces_db -c "GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO mathforces; GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO mathforces;"

echo "======================================"
echo "Компиляция проекта (Release)"
echo "======================================"

if [ ! -f "config.env" ]; then
    cp config.env.example config.env || true
fi

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

echo "======================================"
echo "Готово! Проект собран с чистыми зависимостями Qt."
echo "Пожалуйста, проверьте файл config.env (добавьте ключи)."
echo ""
echo "Для запуска:"
echo "Терминал 1: cd build && ./server/Server"
echo "Терминал 2: cd build && ./client/Client"
echo "======================================"
