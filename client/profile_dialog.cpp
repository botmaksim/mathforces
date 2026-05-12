#include "api_config.h"
#include "profile_dialog.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QFrame>

ProfileDialog::ProfileDialog(const QString& token, int targetUserId, const QString& myRole, QWidget* parent)
    : QDialog(parent), m_token(token), m_targetUserId(targetUserId), m_myRole(myRole), m_isSelf(false), m_canBlog(false), m_myUserId(-1)
{
    setWindowTitle("Профиль пользователя");
    resize(600, 800);
    
    QVBoxLayout* L = new QVBoxLayout(this);
    
    m_lblUsername = new QLabel("Загрузка...", this);
    m_lblUsername->setStyleSheet("font-size: 24px; font-weight: bold;");
    m_lblName = new QLabel(this);
    m_lblRating = new QLabel(this);
    m_lblEmail = new QLabel(this); // can be hidden
    
    L->addWidget(m_lblUsername);
    L->addWidget(m_lblName);
    L->addWidget(m_lblRating);
    L->addWidget(m_lblEmail);
    
    L->addWidget(new QLabel("<b>Блог:</b>"));
    
    m_txtNewPost = new QTextEdit(this);
    m_txtNewPost->setPlaceholderText("О чем вы думаете?");
    m_txtNewPost->setMaximumHeight(80);
    m_txtNewPost->hide();
    
    m_btnPost = new QPushButton("Отправить в блог", this);
    m_btnPost->hide();
    connect(m_btnPost, &QPushButton::clicked, this, &ProfileDialog::addBlogPost);
    
    L->addWidget(m_txtNewPost);
    L->addWidget(m_btnPost);
    
    QScrollArea* sa = new QScrollArea(this);
    sa->setWidgetResizable(true);
    m_blogContainer = new QWidget(sa);
    m_blogLayout = new QVBoxLayout(m_blogContainer);
    m_blogLayout->addStretch();
    sa->setWidget(m_blogContainer);
    L->addWidget(sa);
    
    fetchMyId();
}

void ProfileDialog::fetchMyId() {
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/users/profile")); // my profile
    req.setRawHeader("Authorization", m_token.toUtf8());
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
            m_myUserId = o["id"].toInt();
        }
        r->deleteLater(); m->deleteLater();
        loadProfile();
    });
}

void ProfileDialog::loadProfile() {
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    // target profile
    QNetworkRequest req(QUrl(QString(ApiConfig::baseUrl + "/api/users/profile?id=%1").arg(m_targetUserId)));
    req.setRawHeader("Authorization", m_token.toUtf8());
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QJsonObject o = QJsonDocument::fromJson(r->readAll()).object();
            m_lblUsername->setText(o["username"].toString());
            m_lblName->setText(o["name"].toString());
            m_lblRating->setText("Эло (Рейтинг): " + QString::number(o["rating"].toInt()));
            
            if (o.contains("email")) {
                m_lblEmail->setText("Email: " + o["email"].toString());
            } else {
                m_lblEmail->hide();
            }
            
            m_canBlog = o["can_blog"].toBool();
            m_isSelf = (m_myUserId == m_targetUserId);
            
            if (m_isSelf && m_canBlog) {
                m_txtNewPost->show();
                m_btnPost->show();
            } else {
                m_txtNewPost->hide();
                m_btnPost->hide();
            }
            
            loadBlogPosts();
        } else {
            m_lblUsername->setText("Ошибка загрузки профиля");
        }
        r->deleteLater(); m->deleteLater();
    });
}

void ProfileDialog::loadBlogPosts() {
    QLayoutItem *child;
    while ((child = m_blogLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(QString(ApiConfig::baseUrl + "/api/blog/posts?user_id=%1").arg(m_targetUserId)));
    req.setRawHeader("Authorization", m_token.toUtf8());
    QNetworkReply* r = m->get(req);
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
            for (auto v : arr) {
                QJsonObject o = v.toObject();
                QFrame* f = new QFrame(m_blogContainer);
                f->setFrameShape(QFrame::StyledPanel);
                QVBoxLayout* l = new QVBoxLayout(f);
                QLabel* dateLbl = new QLabel(o["created_at"].toString());
                dateLbl->setStyleSheet("color: gray; font-size: 10px;");
                QLabel* cLbl = new QLabel(o["content"].toString());
                cLbl->setWordWrap(true);
                QPushButton* bComment = new QPushButton("Комментарии");
                int pid = o["id"].toInt();
                connect(bComment, &QPushButton::clicked, [this, pid]() { showComments(pid); });
                l->addWidget(dateLbl);
                l->addWidget(cLbl);
                l->addWidget(bComment);
                m_blogLayout->addWidget(f);
            }
        }
        m_blogLayout->addStretch();
        r->deleteLater(); m->deleteLater();
    });
}

void ProfileDialog::addBlogPost() {
    QString c = m_txtNewPost->toPlainText();
    if (c.isEmpty()) return;
    QNetworkAccessManager* m = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/blog/posts"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", m_token.toUtf8());
    QJsonObject j; j["content"] = c;
    QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
    connect(r, &QNetworkReply::finished, [this, r, m]() {
        if (r->error() == QNetworkReply::NoError) {
            m_txtNewPost->clear();
            loadBlogPosts();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось отправить запись: " + r->errorString());
        }
        r->deleteLater(); m->deleteLater();
    });
}

void ProfileDialog::showComments(int postId) {
    QDialog d(this);
    d.setWindowTitle("Комментарии");
    d.resize(400, 500);
    QVBoxLayout* l = new QVBoxLayout(&d);
    
    QScrollArea* sa = new QScrollArea(&d);
    sa->setWidgetResizable(true);
    QWidget* cw = new QWidget(sa);
    QVBoxLayout* cl = new QVBoxLayout(cw);
    sa->setWidget(cw);
    l->addWidget(sa);
    
    QTextEdit* te = new QTextEdit(&d);
    te->setMaximumHeight(60);
    QPushButton* b = new QPushButton("Отправить", &d);
    l->addWidget(te); l->addWidget(b);
    
    auto loadC = [&]() {
        QLayoutItem* item;
        while ((item = cl->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        QNetworkAccessManager* m = new QNetworkAccessManager(&d);
        QNetworkRequest req(QUrl(QString(ApiConfig::baseUrl + "/api/blog/comments?post_id=%1").arg(postId)));
        req.setRawHeader("Authorization", m_token.toUtf8());
        QNetworkReply* r = m->get(req);
        QObject::connect(r, &QNetworkReply::finished, [&, r, m]() {
            if (r->error() == QNetworkReply::NoError) {
                QJsonArray arr = QJsonDocument::fromJson(r->readAll()).array();
                for (auto v : arr) {
                    QJsonObject o = v.toObject();
                    QLabel* lbl = new QLabel(QString("<b>%1</b> <i>%2</i><br>%3").arg(o["username"].toString(), o["created_at"].toString(), o["content"].toString()));
                    lbl->setWordWrap(true);
                    cl->addWidget(lbl);
                }
                cl->addStretch();
            }
            r->deleteLater(); m->deleteLater();
        });
    };
    
    QObject::connect(b, &QPushButton::clicked, [&]() {
        QString txt = te->toPlainText();
        if (txt.isEmpty()) return;
        QNetworkAccessManager* m = new QNetworkAccessManager(&d);
        QNetworkRequest req(QUrl(ApiConfig::baseUrl + "/api/blog/comments"));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Authorization", m_token.toUtf8());
        QJsonObject j; j["post_id"] = postId; j["content"] = txt;
        QNetworkReply* r = m->post(req, QJsonDocument(j).toJson());
        QObject::connect(r, &QNetworkReply::finished, [&, r, m]() {
             if (r->error() == QNetworkReply::NoError) {
                 te->clear();
                 loadC();
             }
             r->deleteLater(); m->deleteLater();
        });
    });
    
    loadC();
    d.exec();
}
