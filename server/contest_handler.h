#pragma once
#include "llm_client.h"
#include <QHttpServer>

class ContestHandler {
public:
  ContestHandler();
  void registerRoutes(QHttpServer &server);

private:
  LlmClient m_llm;
};
