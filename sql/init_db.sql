DROP TABLE IF EXISTS submissions CASCADE;
DROP TABLE IF EXISTS tasks CASCADE;
DROP TABLE IF EXISTS contests CASCADE;
DROP TABLE IF EXISTS users CASCADE;

CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) DEFAULT 'student' -- 'student' или 'admin'
);

CREATE TABLE contests (
    id SERIAL PRIMARY KEY,
    title VARCHAR(100) NOT NULL,
    description TEXT,
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP NOT NULL
);

CREATE TABLE tasks (
    id SERIAL PRIMARY KEY,
    contest_id INTEGER REFERENCES contests(id) ON DELETE CASCADE,
    title VARCHAR(100) NOT NULL,
    description TEXT NOT NULL,
    max_score INTEGER DEFAULT 100,
    ai_comment TEXT
);

CREATE TABLE submissions (
    id SERIAL PRIMARY KEY,
    task_id INTEGER REFERENCES tasks(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    answer_text TEXT NOT NULL,
    score INTEGER DEFAULT 0,
    feedback TEXT,
    thinking TEXT,
    status VARCHAR(20) DEFAULT 'pending', -- 'pending', 'graded'
    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Тестовые данные (пароль 12345, для примера захардкодим его хэш или просто '12345')
INSERT INTO users (username, password_hash, role) VALUES ('admin', '12345', 'admin');
INSERT INTO users (username, password_hash, role) VALUES ('student', '12345', 'student');

-- Выдаем права пользователю mathforces
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO mathforces;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO mathforces;
