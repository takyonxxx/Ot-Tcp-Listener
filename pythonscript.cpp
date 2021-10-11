#include "pythonscript.h"
#include "emb.h"
#include<QDebug>

PythonScript *PythonScript::theInstance_= nullptr;

PythonScript* PythonScript::getInstance()
{
    if (theInstance_ == nullptr)
    {
        theInstance_ = new PythonScript();
    }
    return theInstance_;
}

PythonScript::PythonScript()
{
    mutex = new QMutex();

    PyImport_AppendInittab("emb", emb::PyInit_emb);
    Py_Initialize();
    PyImport_ImportModule("emb");
}

PythonScript::~PythonScript()
{
    Py_Finalize();
    mutex = nullptr;
}

QString PythonScript::runScript(QString &script)
{
    QString output{};

    // here comes the ***magic***
    std::string buffer;
    {
        // switch sys.stdout to custom handler
        emb::stdout_write_type write = [&buffer] (std::string s) { buffer += s; };
        emb::set_stdout(write);
        PyRun_SimpleString(script.toStdString().c_str());
        emb::reset_stdout();
    }

    // output what was written to buffer object
    output = QString(buffer.c_str());

    return output;
}
