#ifndef PYTHONSCRIPT_H
#define PYTHONSCRIPT_H
#include <QObject>
#include <QMutex>
#include <QProcess>

using namespace std;

class PythonScript
{
public:
    QMutex *mutex{};
    PythonScript();
    ~PythonScript();

    static PythonScript* getInstance();
    QString runScript(QString&);    


 private:
     static PythonScript *theInstance_;
};

#endif // PYTHONSCRIPT_H
