#include "profile_dialog.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

ProfileDialog::ProfileDialog(const QString &token, int targetUserId,
                             const QString &myRole, QWidget *parent)
    : QDialog(parent), m_token(token), m_targetUserId(targetUserId),
      m_myRole(myRole), m_isSelf(false), m_canBlog(false), m_myUserId(-1) {
  setWindowTitle("User Profile");
  resize(720, 820);

  m_presenter = new ProfilePresenter(m_token, this);

  QVBoxLayout *L = new QVBoxLayout(this);
  L->setContentsMargins(22, 22, 22, 22);
  L->setSpacing(12);

  m_lblUsername = new QLabel("Loading...", this);
  m_lblUsername->setObjectName("sectionTitle");
  m_lblName = new QLabel(this);
  m_lblRating = new QLabel(this);
  m_lblEmail = new QLabel(this); // can be hidden

  L->addWidget(m_lblUsername);
  L->addWidget(m_lblName);
  L->addWidget(m_lblRating);
  L->addWidget(m_lblEmail);

  QLabel *blogTitle = new QLabel("Blog", this);
  blogTitle->setObjectName("sectionTitle");
  L->addWidget(blogTitle);

  m_txtNewPost = new QTextEdit(this);
  m_txtNewPost->setPlaceholderText("What are you thinking about?");
  m_txtNewPost->setMaximumHeight(80);
  m_txtNewPost->hide();

  m_btnPost = new QPushButton("Post to blog", this);
  m_btnPost->hide();
  connect(m_btnPost, &QPushButton::clicked, this, &ProfileDialog::addBlogPost);

  L->addWidget(m_txtNewPost);
  L->addWidget(m_btnPost);

  QScrollArea *sa = new QScrollArea(this);
  sa->setWidgetResizable(true);
  m_blogContainer = new QWidget(sa);
  m_blogLayout = new QVBoxLayout(m_blogContainer);
  m_blogLayout->addStretch();
  sa->setWidget(m_blogContainer);
  L->addWidget(sa);

  // Connection logic
  connect(m_presenter, &ProfilePresenter::myIdLoaded, this, [this](int id){
      m_myUserId = id;
      if (m_targetUserId == -1) {
          m_targetUserId = id;
      }
      loadProfile();
  });
  
  connect(m_presenter, &ProfilePresenter::profileLoaded, this, [this](const QJsonObject& o){
      m_lblUsername->setText(o["username"].toString());
      m_lblName->setText(o["name"].toString());
      m_lblRating->setText("Elo (Rating): " + QString::number(o["rating"].toInt()));

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
  });
  
  connect(m_presenter, &ProfilePresenter::blogPostsLoaded, this, [this](const QJsonArray& arr){
      QLayoutItem *child;
      while ((child = m_blogLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
      }
      for (auto v : arr) {
        QJsonObject o = v.toObject();
        QFrame *f = new QFrame(m_blogContainer);
        f->setObjectName("softCard");
        QVBoxLayout *l = new QVBoxLayout(f);
        QLabel *dateLbl = new QLabel(o["created_at"].toString());
        dateLbl->setObjectName("mutedLabel");
        QLabel *cLbl = new QLabel(o["content"].toString());
        cLbl->setWordWrap(true);
        QPushButton *bComment = new QPushButton("Comments");
        int pid = o["id"].toInt();
        connect(bComment, &QPushButton::clicked, [this, pid]() { showComments(pid); });
        l->addWidget(dateLbl);
        l->addWidget(cLbl);
        l->addWidget(bComment);
        m_blogLayout->addWidget(f);
      }
      m_blogLayout->addStretch();
  });
  
  connect(m_presenter, &ProfilePresenter::blogPostAdded, this, [this](){
      m_txtNewPost->clear();
      loadBlogPosts();
  });
  
  connect(m_presenter, &ProfilePresenter::errorOccurred, this, [this](const QString& err){
      QMessageBox::warning(this, "Error", err);
  });

  fetchMyId();
}

void ProfileDialog::fetchMyId() {
  m_presenter->fetchMyId();
}

void ProfileDialog::loadProfile() {
  int idToLoad = (m_targetUserId == -1) ? m_myUserId : m_targetUserId;
  m_presenter->loadProfile(idToLoad);
}

void ProfileDialog::loadBlogPosts() {
  int idToLoad = (m_targetUserId == -1) ? m_myUserId : m_targetUserId;
  m_presenter->loadBlogPosts(idToLoad);
}

void ProfileDialog::addBlogPost() {
  QString c = m_txtNewPost->toPlainText();
  if (c.isEmpty()) return;
  m_presenter->addBlogPost(c);
}

void ProfileDialog::showComments(int postId) {
  QDialog d(this);
  d.setWindowTitle("Comments");
  d.resize(400, 500);
  QVBoxLayout *l = new QVBoxLayout(&d);

  QScrollArea *sa = new QScrollArea(&d);
  sa->setWidgetResizable(true);
  QWidget *cw = new QWidget(sa);
  QVBoxLayout *cl = new QVBoxLayout(cw);
  sa->setWidget(cw);
  l->addWidget(sa);

  QTextEdit *te = new QTextEdit(&d);
  te->setMaximumHeight(60);
  QPushButton *b = new QPushButton("Send", &d);
  l->addWidget(te);
  l->addWidget(b);

  auto loadC = [this, postId]() {
      m_presenter->loadComments(postId);
  };
  
  auto connLoader = connect(m_presenter, &ProfilePresenter::commentsLoaded, [cl](const QJsonArray& arr){
      QLayoutItem *item;
      while ((item = cl->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
      }
      for (auto v : arr) {
        QJsonObject o = v.toObject();
        QLabel *lbl = new QLabel(QString("<b>%1</b> <i>%2</i><br>%3")
                                     .arg(o["username"].toString(),
                                          o["created_at"].toString(),
                                          o["content"].toString()));
        lbl->setWordWrap(true);
        cl->addWidget(lbl);
      }
      cl->addStretch();
  });
  
  auto connAdder = connect(m_presenter, &ProfilePresenter::commentAdded, [loadC, te](){
      te->clear();
      loadC();
  });

  QObject::connect(b, &QPushButton::clicked, [this, postId, te]() {
      QString txt = te->toPlainText();
      if (txt.isEmpty()) return;
      m_presenter->addComment(postId, txt);
  });

  loadC();
  d.exec();
  
  disconnect(connLoader);
  disconnect(connAdder);
}
