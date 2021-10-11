#include "logger.h"

Logger *Logger::theInstance_= nullptr;

Logger* Logger::getInstance()
{
    return theInstance_;
}


Logger::Logger(QObject *parent, QString fileName, bool showDate, bool debug)
    : QObject(parent), m_showDate(showDate), m_Debug(debug)
{
    if (!fileName.isEmpty()) {
        file = new QFile;
        file->setFileName(fileName);
        file->open(QIODevice::Append | QIODevice::Text);
    }

    theInstance_ = this;
}

void Logger::write(const QString &value) {

    if (!file)
        return;

    QString text = value;// + "";
    if (m_showDate)
        text = QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss ") + text + "\n";

    if (m_Debug)
    {
        QTextStream out(file);
        out.setCodec("UTF-8");
        if (file != 0) {
            out << text;
        }
    }

    fprintf(stderr,"%s",text.toStdString().c_str());
}

void Logger::setShowDateTime(bool value) {
    m_showDate = value;
}

void Logger::setDebug(bool Debug)
{
    m_Debug = Debug;
}

Logger::~Logger() {
    if (file != 0)
        file->close();
}
