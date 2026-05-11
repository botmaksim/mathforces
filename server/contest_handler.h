#pragma once
#include <QHttpServer>
#include "llm_client.h"

class ContestHandler {
public:
    ContestHandler();
    void registerRoutes(QHttpServer& server);
private:
    LlmClient m_llm;
};
