DROP TABLE IF EXISTS hacks CASCADE;
DROP TABLE IF EXISTS virtual_participations CASCADE;
DROP TABLE IF EXISTS comments CASCADE;
DROP TABLE IF EXISTS blogs CASCADE;
DROP TABLE IF EXISTS contest_participants CASCADE;
DROP TABLE IF EXISTS submissions CASCADE;
DROP TABLE IF EXISTS tasks CASCADE;
DROP TABLE IF EXISTS contests CASCADE;
DROP TABLE IF EXISTS users CASCADE;

CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE,
    email VARCHAR(100) UNIQUE,
    google_id VARCHAR(100) UNIQUE,
    password_hash VARCHAR(255),
    name VARCHAR(100),
    role VARCHAR(20) DEFAULT 'student', -- 'superadmin', 'admin', 'moderator', 'student'
    is_banned BOOLEAN DEFAULT FALSE,
    rating INTEGER DEFAULT 500,
    reputation INTEGER DEFAULT 0,
    hidden_probability FLOAT DEFAULT 0.0,
    can_blog BOOLEAN DEFAULT TRUE
);

CREATE TABLE contests (
    id SERIAL PRIMARY KEY,
    author_id INTEGER REFERENCES users(id) ON DELETE SET NULL,
    title VARCHAR(100) NOT NULL,
    description TEXT,
    start_time TIMESTAMP NOT NULL,
    duration_hours FLOAT NOT NULL DEFAULT 2.0,
    end_time TIMESTAMP NOT NULL,
    is_published BOOLEAN DEFAULT FALSE
);

CREATE TABLE tasks (
    id SERIAL PRIMARY KEY,
    contest_id INTEGER REFERENCES contests(id) ON DELETE CASCADE,
    task_type VARCHAR(20) DEFAULT 'solution', -- 'answer_only' or 'solution'
    title VARCHAR(100) NOT NULL,
    description TEXT NOT NULL,
    max_score INTEGER DEFAULT 100,
    max_submissions INTEGER DEFAULT 10,
    correct_answer TEXT,
    editorial TEXT,
    ai_comment TEXT,
    send_editorial_to_ai BOOLEAN DEFAULT FALSE,
    tags TEXT DEFAULT '',
    difficulty INTEGER DEFAULT 1000
);

CREATE TABLE submissions (
    id SERIAL PRIMARY KEY,
    task_id INTEGER REFERENCES tasks(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    answer_text TEXT NOT NULL,
    score INTEGER DEFAULT 0,
    feedback TEXT,
    thinking TEXT,
    ai_probability FLOAT,
    status VARCHAR(20) DEFAULT 'pending', -- 'pending', 'graded'
    is_upsolving BOOLEAN DEFAULT FALSE,
    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE virtual_participations (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    contest_id INTEGER REFERENCES contests(id) ON DELETE CASCADE,
    start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(contest_id, user_id)
);

CREATE TABLE hacks (
    id SERIAL PRIMARY KEY,
    hacker_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    submission_id INTEGER REFERENCES submissions(id) ON DELETE CASCADE,
    hack_text TEXT NOT NULL,
    status VARCHAR(20) DEFAULT 'pending', -- 'pending', 'successful', 'unsuccessful'
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE contest_participants (
    id SERIAL PRIMARY KEY,
    contest_id INTEGER REFERENCES contests(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    is_official BOOLEAN DEFAULT TRUE,
    UNIQUE(contest_id, user_id)
);

CREATE TABLE blogs (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    title VARCHAR(200) NOT NULL,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE comments (
    id SERIAL PRIMARY KEY,
    blog_id INTEGER REFERENCES blogs(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    content TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Тестовые данные
INSERT INTO users (username, email, password_hash, role, name) VALUES ('superadmin', 'superadmin@example.com', '12345', 'superadmin', 'Super Admin');
INSERT INTO users (username, email, password_hash, role, name) VALUES ('admin', 'admin@example.com', '12345', 'admin', 'Admin');
INSERT INTO users (username, email, password_hash, role, name) VALUES ('student', 'student@example.com', '12345', 'student', 'Student');

-- Выдаем права пользователю mathforces
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO mathforces;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO mathforces;

ALTER TABLE contests ADD COLUMN IF NOT EXISTS is_published BOOLEAN DEFAULT FALSE;
