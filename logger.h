#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QDateTime>
#include <QTextStream>

class Logger : public QObject
{
 Q_OBJECT
public:
 explicit Logger(QObject *parent, QString fileName, bool showDate, bool debug);
 ~Logger();

 static Logger* getInstance();
 void setShowDateTime(bool value);

 void setDebug(bool Debug);

private:
 QFile *file;
 bool m_showDate;
 bool m_Debug;
 static Logger *theInstance_;

public slots:
 void write(const QString &value);

};

#endif // LOGGER_H
